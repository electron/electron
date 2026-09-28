// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/electron_prefetch_mac.h"

#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <libkern/OSByteOrder.h>
#include <mach-o/fat.h>
#include <mach-o/getsect.h>
#include <mach-o/loader.h>
#include <mach/mach.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace electron {

namespace {

// The slice of a universal binary this process runs, and the name of the
// other architecture's V8 snapshot to skip.
#if defined(__aarch64__)
constexpr cpu_type_t kCpuType = CPU_TYPE_ARM64;
constexpr std::string_view kOtherArchName = "x86_64";
#else
constexpr cpu_type_t kCpuType = CPU_TYPE_X86_64;
constexpr std::string_view kOtherArchName = "arm64";
#endif

// Don't prefetch unless this much memory could be handed out without paging.
constexpr uint64_t kMinAvailableMemory = 512 * 1024 * 1024;

struct FileRange {
  std::string path;
  off_t begin = 0;
  off_t end = 0;
};

// Samples the framework's __TEXT segment with mincore(); a warm framework has
// nearly every page resident, a cold one almost none.
bool IsFrameworkTextResident(const mach_header_64* header) {
  unsigned long size = 0;
  const uint8_t* text = getsegmentdata(header, "__TEXT", &size);
  if (!text || size == 0)
    return true;

  constexpr size_t kProbes = 64;
  const uintptr_t page_mask = ~static_cast<uintptr_t>(getpagesize() - 1);
  size_t resident = 0;
  for (size_t i = 0; i < kProbes; ++i) {
    const uintptr_t addr =
        (reinterpret_cast<uintptr_t>(text) + (size / kProbes) * i) & page_mask;
    char vec = 0;
    if (mincore(reinterpret_cast<void*>(addr), 1, &vec) == 0 &&
        (vec & MINCORE_INCORE)) {
      ++resident;
    }
  }
  return resident * 10 >= kProbes * 9;
}

// Prefetching evicts other cached files, so only do it when memory is
// plentiful.
bool HasMemoryToSpare() {
  int pressure = 0;
  size_t pressure_size = sizeof(pressure);
  if (sysctlbyname("kern.memorystatus_vm_pressure_level", &pressure,
                   &pressure_size, nullptr, 0) == 0 &&
      pressure > 1) {
    return false;
  }

  vm_statistics64_data_t vm_stats;
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  const mach_port_t host = mach_host_self();
  const kern_return_t kr = host_statistics64(
      host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vm_stats), &count);
  mach_port_deallocate(mach_task_self(), host);
  if (kr != KERN_SUCCESS)
    return false;
  const uint64_t available_pages =
      static_cast<uint64_t>(vm_stats.free_count) + vm_stats.inactive_count +
      vm_stats.speculative_count + vm_stats.purgeable_count;
  return available_pages * vm_page_size >= kMinAvailableMemory;
}

// The byte range of |path| that this architecture maps: its slice of a
// universal binary, otherwise the whole file.
FileRange RangeForThisArch(const std::string& path) {
  FileRange range{path, 0, 0};
  const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return range;
  range.end = lseek(fd, 0, SEEK_END);

  fat_header header;
  if (pread(fd, &header, sizeof(header), 0) == sizeof(header)) {
    const uint32_t magic = OSSwapBigToHostInt32(header.magic);
    const uint32_t nfat_arch = OSSwapBigToHostInt32(header.nfat_arch);
    if ((magic == FAT_MAGIC || magic == FAT_MAGIC_64) && nfat_arch < 16) {
      const bool is_64 = magic == FAT_MAGIC_64;
      const size_t entry_size = is_64 ? sizeof(fat_arch_64) : sizeof(fat_arch);
      for (uint32_t i = 0; i < nfat_arch; ++i) {
        const off_t entry_offset = sizeof(fat_header) + i * entry_size;
        cpu_type_t cputype = 0;
        uint64_t offset = 0;
        uint64_t size = 0;
        if (is_64) {
          fat_arch_64 arch;
          if (pread(fd, &arch, sizeof(arch), entry_offset) != sizeof(arch))
            break;
          cputype = OSSwapBigToHostInt32(arch.cputype);
          offset = OSSwapBigToHostInt64(arch.offset);
          size = OSSwapBigToHostInt64(arch.size);
        } else {
          fat_arch arch;
          if (pread(fd, &arch, sizeof(arch), entry_offset) != sizeof(arch))
            break;
          cputype = OSSwapBigToHostInt32(arch.cputype);
          offset = OSSwapBigToHostInt32(arch.offset);
          size = OSSwapBigToHostInt32(arch.size);
        }
        if (cputype == kCpuType) {
          range.begin = static_cast<off_t>(offset);
          range.end = std::min<off_t>(range.end, offset + size);
          break;
        }
      }
    }
  }
  close(fd);
  return range;
}

void ReadRange(const FileRange& range, std::vector<char>& buffer) {
  const int fd = open(range.path.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return;
  for (off_t offset = range.begin; offset < range.end;) {
    const size_t want =
        static_cast<size_t>(std::min<off_t>(buffer.size(), range.end - offset));
    const ssize_t n = pread(fd, buffer.data(), want, offset);
    if (n <= 0)
      break;
    offset += n;
  }
  close(fd);
}

void* PrefetchThreadMain(void* arg) {
  // Owned by this thread; see PrefetchFrameworkFilesIfCold().
  const std::string binary_path = *static_cast<std::string*>(arg);
  delete static_cast<std::string*>(arg);

  // Yield the disk to the main thread's own page faults, which matters on
  // slow disks; on SSDs this costs none of the gain.
  setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD, IOPOL_UTILITY);
  if (!HasMemoryToSpare())
    return nullptr;

  // .../Electron Framework.framework/Versions/A/Electron Framework
  const std::string version_dir =
      binary_path.substr(0, binary_path.rfind('/') + 1);

  std::vector<FileRange> ranges = {
      RangeForThisArch(binary_path),
      RangeForThisArch(version_dir + "Libraries/libffmpeg.dylib")};

  // The V8 snapshot, ICU data and the resource paks, skipping the other
  // architecture's snapshot in a universal build. Locale paks live in
  // subdirectories and are small, so they're left to demand paging.
  const std::string resources_dir = version_dir + "Resources/";
  if (DIR* dir = opendir(resources_dir.c_str())) {
    while (const dirent* entry = readdir(dir)) {
      if (entry->d_type != DT_REG)
        continue;
      const std::string name(entry->d_name);
      if (!name.ends_with(".pak") && !name.ends_with(".dat") &&
          !name.ends_with(".bin")) {
        continue;
      }
      if (name.find(std::string(".") + std::string(kOtherArchName) + ".") !=
          std::string::npos) {
        continue;
      }
      ranges.push_back(RangeForThisArch(resources_dir + name));
    }
    closedir(dir);
  }

  // Plain sequential reads: they populate the page cache in large chunks
  // without mapping anything into this process.
  std::vector<char> buffer(8 * 1024 * 1024);
  for (const FileRange& range : ranges)
    ReadRange(range, buffer);
  return nullptr;
}

}  // namespace

void PrefetchFrameworkFilesIfCold() {
  Dl_info info;
  if (!dladdr(reinterpret_cast<const void*>(&PrefetchFrameworkFilesIfCold),
              &info) ||
      !info.dli_fname || !info.dli_fbase) {
    return;
  }
  if (IsFrameworkTextResident(
          static_cast<const mach_header_64*>(info.dli_fbase))) {
    return;
  }

  // This runs before base is initialized, so use a raw detached pthread.
  auto* binary_path = new std::string(info.dli_fname);
  pthread_t thread;
  if (pthread_create(&thread, nullptr, &PrefetchThreadMain, binary_path) != 0) {
    delete binary_path;
    return;
  }
  pthread_detach(thread);
}

}  // namespace electron
