// Double-open errno-87 repro in plain C / Win32, to rule Go in/out.
//
// Build (cross, macOS/Linux):
//   zig cc -target x86_64-windows-gnu -O2 -o repro_c.exe repro.c
// or on Windows:
//   cl /O2 repro.c /Fe:repro_c.exe
//
// Usage:
//   cd C:\work
//   repro_c.exe src\third_party 32 12
//     arg1 = relative dir to walk for files (.h only)
//     arg2 = worker threads (default 32)
//     arg3 = rounds (default 12)

#define WIN32_LEAN_AND_MEAN
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_FILES 200000

static wchar_t* g_files[MAX_FILES];
static volatile LONG g_nfiles = 0;
static volatile LONG g_idx;
static volatile LONG g_err87 = 0;
static volatile LONG g_err87_recovered = 0;
static volatile LONG g_other = 0;
static HANDLE g_inner_sema;

static void walk(const wchar_t* dir) {
  wchar_t pat[1024];
  _snwprintf(pat, 1024, L"%ls\\*", dir);
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW(pat, &fd);
  if (h == INVALID_HANDLE_VALUE)
    return;
  do {
    if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L".."))
      continue;
    wchar_t full[1024];
    _snwprintf(full, 1024, L"%ls\\%ls", dir, fd.cFileName);
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      walk(full);
    } else {
      size_t n = wcslen(full);
      if (n > 2 && !_wcsicmp(full + n - 2, L".h") && g_nfiles < MAX_FILES) {
        g_files[g_nfiles] = _wcsdup(full);
        InterlockedIncrement(&g_nfiles);
      }
    }
  } while (FindNextFileW(h, &fd));
  FindClose(h);
}

static HANDLE open_ro(const wchar_t* p) {
  return CreateFileW(p, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

struct inner_arg {
  const wchar_t* path;
  volatile LONG done;
  DWORD err;
};

static unsigned __stdcall inner_thread(void* arg) {
  struct inner_arg* a = (struct inner_arg*)arg;
  WaitForSingleObject(g_inner_sema, INFINITE);
  HANDLE h = open_ro(a->path);
  if (h == INVALID_HANDLE_VALUE) {
    a->err = GetLastError();
  } else {
    a->err = 0;
    CloseHandle(h);
  }
  ReleaseSemaphore(g_inner_sema, 1, NULL);
  InterlockedExchange(&a->done, 1);
  return 0;
}

static unsigned __stdcall worker(void* unused) {
  (void)unused;
  for (;;) {
    LONG i = InterlockedIncrement(&g_idx) - 1;
    if (i >= g_nfiles)
      return 0;
    const wchar_t* p = g_files[i];

    HANDLE h1 = open_ro(p);
    if (h1 == INVALID_HANDLE_VALUE) {
      DWORD e = GetLastError();
      if (e == ERROR_INVALID_PARAMETER)
        InterlockedIncrement(&g_err87);
      else
        InterlockedIncrement(&g_other);
      continue;
    }

    struct inner_arg a = {p, 0, 0};
    HANDLE th = (HANDLE)_beginthreadex(NULL, 0, inner_thread, &a, 0, NULL);
    WaitForSingleObject(th, INFINITE);
    CloseHandle(th);

    if (a.err == ERROR_INVALID_PARAMETER) {
      InterlockedIncrement(&g_err87);
      Sleep(5);
      HANDLE h2 = open_ro(p);
      if (h2 != INVALID_HANDLE_VALUE) {
        InterlockedIncrement(&g_err87_recovered);
        CloseHandle(h2);
      }
      fwprintf(stderr, L"[c] errno87 %ls\n", p);
    } else if (a.err != 0) {
      InterlockedIncrement(&g_other);
    }
    CloseHandle(h1);
  }
}

int wmain(int argc, wchar_t** argv) {
  const wchar_t* dir = (argc > 1) ? argv[1] : L"src\\third_party";
  int nworkers = (argc > 2) ? _wtoi(argv[2]) : 32;
  int rounds = (argc > 3) ? _wtoi(argv[3]) : 12;

  wprintf(L"dir=%ls workers=%d rounds=%d\n", dir, nworkers, rounds);
  walk(dir);
  wprintf(L"files=%ld\n", g_nfiles);
  if (g_nfiles == 0)
    return 1;

  g_inner_sema = CreateSemaphoreW(NULL, nworkers, nworkers, NULL);
  HANDLE* ths = (HANDLE*)calloc(nworkers, sizeof(HANDLE));

  for (int r = 1; r <= rounds; r++) {
    g_idx = 0;
    DWORD t0 = GetTickCount();
    for (int w = 0; w < nworkers; w++)
      ths[w] = (HANDLE)_beginthreadex(NULL, 0, worker, NULL, 0, NULL);
    for (int w = 0; w < nworkers; w++) {
      WaitForSingleObject(ths[w], INFINITE);
      CloseHandle(ths[w]);
    }
    wprintf(L"round %d/%d: %ld opens in %lums (err87 so far: %ld)\n", r, rounds,
            g_nfiles, GetTickCount() - t0, g_err87);
  }

  wprintf(L"\n=== summary ===\nopens=%ld err87=%ld recovered=%ld other=%ld\n",
          (long)g_nfiles * rounds, g_err87, g_err87_recovered, g_other);
  return 0;
}
