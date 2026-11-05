/* CFI-instrumented host (stands in for Electron/Node): links libuv statically in the
 * same ThinLTO unit, exports the uv_* API, dlopens the uninstrumented addon. */
#include <uv.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef int (*addon_fn)(uv_loop_t*);

/* Like node::binding::CallAddonEntryPoint: the one unchecked host->addon call. */
__attribute__((no_sanitize("cfi-icall"))) static int call_addon(addon_fn f, uv_loop_t* l) { return f(l); }

static void* h_malloc(size_t n) { return malloc(n); }
static void* h_realloc(void* p, size_t n) { return realloc(p, n); }
static void* h_calloc(size_t a, size_t b) { return calloc(a, b); }
static void h_free(void* p) { free(p); }
/* Wrongly typed internal callback: protected internal call sites must still trap. */
static int bad_malloc(int a, int b) { return a + b; }

int main(int argc, char** argv) {
  const char* s = argc > 2 ? argv[2] : "";
  uv_loop_t* loop = uv_default_loop();
  fprintf(stderr, "[%s]\n", s);
  if (!strcmp(s, "preadv") || !strcmp(s, "mkstemp") || !strcmp(s, "getsockname") ||
      !strcmp(s, "alloc_ok") || !strcmp(s, "alloc_bad")) {
    uv_fs_t r; char a[4], b[4]; uv_buf_t bufs[2] = {uv_buf_init(a, 4), uv_buf_init(b, 4)};
    if (!strcmp(s, "preadv")) {  /* positional read with 2 buffers -> uv__preadv -> dlsym'd preadv64 */
      int fd = open("/etc/passwd", O_RDONLY);
      int n = uv_fs_read(NULL, &r, fd, bufs, 2, 0, NULL);
      fprintf(stderr, "  host: uv_fs_read(2 bufs, off 0) = %d\n", n);
    } else if (!strcmp(s, "mkstemp")) {
      char tpl[] = "/tmp/cfiXXXXXX";
      int n = uv_fs_mkstemp(NULL, &r, tpl, NULL);
      fprintf(stderr, "  host: uv_fs_mkstemp = %d %s\n", n, r.path); unlink(r.path); uv_fs_req_cleanup(&r);
    } else if (!strcmp(s, "getsockname")) {
      uv_tcp_t t; struct sockaddr_storage ss; int len = sizeof ss; struct sockaddr_in a4;
      uv_ip4_addr("127.0.0.1", 0, &a4); uv_tcp_init(loop, &t); uv_tcp_bind(&t, (struct sockaddr*)&a4, 0);
      fprintf(stderr, "  host: uv_tcp_getsockname = %d (via fn pointer to libc getsockname)\n",
              uv_tcp_getsockname(&t, (struct sockaddr*)&ss, &len));
    } else if (!strcmp(s, "alloc_ok")) {
      uv_replace_allocator(h_malloc, h_realloc, h_calloc, h_free);
      uv_fs_t r2; uv_fs_realpath(NULL, &r2, "/tmp", NULL); uv_fs_req_cleanup(&r2);
      fprintf(stderr, "  host: correctly typed allocator ok\n");
    } else {
      uv_replace_allocator((uv_malloc_func)bad_malloc, h_realloc, h_calloc, h_free);
      uv_fs_t r2; uv_fs_realpath(NULL, &r2, "/tmp", NULL);
      fprintf(stderr, "  host: wrongly typed allocator was NOT caught\n");
    }
    fprintf(stderr, "OK %s\n", s);
    return 0;
  }
  void* lib = dlopen(argv[1], RTLD_NOW);
  if (!lib) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 2; }
  char sym[64]; snprintf(sym, sizeof sym, "addon_%s", s);
  addon_fn f = (addon_fn)dlsym(lib, sym);
  if (!f) { fprintf(stderr, "no %s\n", sym); return 2; }
  int r = call_addon(f, loop);
  if (!strcmp(s, "allocator")) { void* p = uv_loop_new(); (void)p; }  /* uv__malloc -> addon a_malloc */
  uv_run(loop, UV_RUN_DEFAULT);
  fprintf(stderr, "OK %s (rc=%d)\n", s, r);
  return 0;
}
