// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// Thread-safe setenv(), unsetenv(), putenv() and clearenv().
//
// Before glibc 2.41, adding a variable reallocates the `environ` array and
// frees the old one. A getenv() running on another thread at that moment
// reads freed memory. Electron cannot avoid this by ordering: Chromium's
// thread pool and system libraries call getenv() at any time, and app code
// adds variables through process.env whenever it likes.
//
// Because the executable is linked with -rdynamic, these definitions replace
// glibc's for the whole process, including shared libraries such as GTK. They
// follow the scheme glibc 2.41 adopted upstream:
//
//  - An `environ` array that has been published is never freed. Growing it
//    allocates a larger array, copies the entries and publishes the new one.
//    Capacity doubles, so the arrays that are given up total less than the
//    live one.
//  - "NAME=value" strings are never freed, as in every glibc version, because
//    getenv() callers hold pointers into them. Identical strings are reused,
//    so setting the same value again allocates nothing.
//
// A getenv() on another thread therefore sees either the old or the new state,
// and every pointer it reads stays valid.

#ifdef UNSAFE_BUFFERS_BUILD
// These are libc functions over `environ`, a null-terminated array of C
// strings with no length to check against.
#pragma allow_unsafe_buffers
#pragma allow_unsafe_libc_calls
#endif

#include <errno.h>
#include <pthread.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>

#include "base/memory/raw_ptr_exclusion.h"

// MemorySanitizer intercepts these functions to track `environ`; leave them
// alone in that configuration.
#if !defined(MEMORY_SANITIZER)

namespace {

pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

// The array this file allocated and last published, and its size in slots.
// Anything else in `environ` is treated as read-only and copied before a
// change.
char** g_owned_array = nullptr;
size_t g_owned_capacity = 0;

// Arrays this file published and then replaced. They are kept on a list, as
// glibc does, so they stay reachable and leak checkers do not report them.
struct RetiredArray {
  // Never freed, so raw_ptr's use-after-free protection has nothing to do.
  RAW_PTR_EXCLUSION char** array;
  RAW_PTR_EXCLUSION RetiredArray* next;
};
RetiredArray* g_retired_arrays = nullptr;

// Every "NAME=value" string setenv() has allocated, keyed by its contents.
void* g_known_strings = nullptr;

class ScopedLock {
 public:
  ScopedLock() { pthread_mutex_lock(&g_lock); }
  ~ScopedLock() { pthread_mutex_unlock(&g_lock); }
  ScopedLock(const ScopedLock&) = delete;
  ScopedLock& operator=(const ScopedLock&) = delete;
};

void PublishArray(char** array) {
  __atomic_store_n(&environ, array, __ATOMIC_RELEASE);
}

void StoreEntry(char** slot, char* entry) {
  __atomic_store_n(slot, entry, __ATOMIC_RELEASE);
}

size_t CountEntries(char** array) {
  size_t count = 0;
  if (array) {
    while (array[count]) {
      ++count;
    }
  }
  return count;
}

// Returns the slot holding `name`, or nullptr.
char** FindEntry(const char* name, size_t name_len) {
  char** array = environ;
  if (!array) {
    return nullptr;
  }
  for (char** slot = array; *slot; ++slot) {
    if (strncmp(*slot, name, name_len) == 0 && (*slot)[name_len] == '=') {
      return slot;
    }
  }
  return nullptr;
}

// Appends `entry`, growing into a new array when the current one is full or
// is not ours. Returns false when memory runs out.
bool AppendEntry(char* entry) {
  char** array = environ;
  const size_t count = CountEntries(array);

  // Slot `count` takes the entry and slot `count + 1` the terminator.
  if (array != g_owned_array || count + 2 > g_owned_capacity) {
    const size_t capacity = std::max(g_owned_capacity * 2, count + 2 + 16);
    // calloc leaves every unused slot null, so the terminator is in place
    // before the entry becomes visible.
    auto** grown = static_cast<char**>(calloc(capacity, sizeof(char*)));
    if (!grown) {
      return false;
    }
    if (g_owned_array) {
      auto* retired = static_cast<RetiredArray*>(malloc(sizeof(RetiredArray)));
      if (!retired) {
        free(grown);
        return false;
      }
      *retired = {g_owned_array, g_retired_arrays};
      g_retired_arrays = retired;
    }
    if (count) {
      memcpy(grown, array, count * sizeof(char*));
    }
    grown[count] = entry;
    g_owned_array = grown;
    g_owned_capacity = capacity;
    PublishArray(grown);
    return true;
  }

  StoreEntry(&array[count + 1], nullptr);
  StoreEntry(&array[count], entry);
  return true;
}

int CompareStrings(const void* a, const void* b) {
  return strcmp(static_cast<const char*>(a), static_cast<const char*>(b));
}

// Returns a "NAME=value" string that stays valid for the life of the process.
char* MakeEntry(const char* name, size_t name_len, const char* value) {
  const size_t value_len = strlen(value);
  auto* entry = static_cast<char*>(malloc(name_len + 1 + value_len + 1));
  if (!entry) {
    return nullptr;
  }
  memcpy(entry, name, name_len);
  entry[name_len] = '=';
  memcpy(entry + name_len + 1, value, value_len + 1);

  void* node = tsearch(entry, &g_known_strings, CompareStrings);
  if (!node) {
    // Not remembered, which only means it will not be reused later.
    return entry;
  }
  char* known = *static_cast<char**>(node);
  if (known != entry) {
    free(entry);
  }
  return known;
}

bool IsValidName(const char* name) {
  return name && name[0] != '\0' && !strchr(name, '=');
}

// Removes every entry for `name`. Entries shift down in place; a reader may
// see one twice or miss one, but every pointer it reads stays valid.
void RemoveEntries(const char* name, size_t name_len) {
  char** array = environ;
  if (!array) {
    return;
  }
  char** slot = array;
  while (*slot) {
    if (strncmp(*slot, name, name_len) == 0 && (*slot)[name_len] == '=') {
      char** to = slot;
      do {
        StoreEntry(to, to[1]);
      } while (*to++);
    } else {
      ++slot;
    }
  }
}

}  // namespace

extern "C" {

__attribute__((visibility("default"))) int setenv(const char* name,
                                                  const char* value,
                                                  int replace) noexcept {
  if (!IsValidName(name)) {
    errno = EINVAL;
    return -1;
  }
  const size_t name_len = strlen(name);

  ScopedLock lock;
  char** existing = FindEntry(name, name_len);
  if (existing && !replace) {
    return 0;
  }
  char* entry = MakeEntry(name, name_len, value);
  if (!entry) {
    errno = ENOMEM;
    return -1;
  }
  if (existing) {
    StoreEntry(existing, entry);
    return 0;
  }
  if (!AppendEntry(entry)) {
    errno = ENOMEM;
    return -1;
  }
  return 0;
}

__attribute__((visibility("default"))) int unsetenv(const char* name) noexcept {
  if (!IsValidName(name)) {
    errno = EINVAL;
    return -1;
  }
  ScopedLock lock;
  RemoveEntries(name, strlen(name));
  return 0;
}

__attribute__((visibility("default"))) int putenv(char* string) noexcept {
  const char* equals = strchr(string, '=');
  ScopedLock lock;
  if (!equals) {
    // glibc removes the variable when the string has no '='.
    RemoveEntries(string, strlen(string));
    return 0;
  }
  const auto name_len = static_cast<size_t>(equals - string);
  if (char** existing = FindEntry(string, name_len)) {
    StoreEntry(existing, string);
    return 0;
  }
  if (!AppendEntry(string)) {
    errno = ENOMEM;
    return -1;
  }
  return 0;
}

__attribute__((visibility("default"))) int clearenv() noexcept {
  ScopedLock lock;
  // The array is dropped, not freed, like every array this file gives up.
  PublishArray(nullptr);
  return 0;
}

}  // extern "C"

#endif  // !defined(MEMORY_SANITIZER)
