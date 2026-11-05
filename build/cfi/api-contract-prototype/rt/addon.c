/* Uninstrumented "native addon": built WITHOUT CFI, registers its own callbacks
 * with the CFI-instrumented libuv exported by the host. */
#include <uv.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define HIT(name) do { fprintf(stderr, "  addon cb: %s\n", name); fflush(stderr); } while (0)

static void close_cb(uv_handle_t* h) { HIT("close_cb (uv__finish_close)"); }

/* work */
static uv_work_t work_req;
static void work_cb(uv_work_t* r) { HIT("work_cb (uv__queue_work)"); }
static void after_work_cb(uv_work_t* r, int s) { HIT("after_work_cb (uv__queue_done)"); }
int addon_work(uv_loop_t* l) { return uv_queue_work(l, &work_req, work_cb, after_work_cb); }

/* timer + close */
static uv_timer_t timer;
static void timer_cb(uv_timer_t* t) { HIT("timer_cb (uv__run_timers)"); uv_close((uv_handle_t*)t, close_cb); }
int addon_timer(uv_loop_t* l) { uv_timer_init(l, &timer); return uv_timer_start(&timer, timer_cb, 1, 0); }

/* async */
static uv_async_t async_h;
static void async_cb(uv_async_t* a) { HIT("async_cb (uv__async_io)"); uv_close((uv_handle_t*)a, NULL); }
int addon_async(uv_loop_t* l) { uv_async_init(l, &async_h, async_cb); return uv_async_send(&async_h); }

/* idle/prepare/check + walk */
static uv_idle_t idle_h; static uv_prepare_t prep_h; static uv_check_t check_h;
static void idle_cb(uv_idle_t* h) { HIT("idle_cb (uv__run_idle)"); uv_close((uv_handle_t*)h, NULL); }
static void prep_cb(uv_prepare_t* h) { HIT("prepare_cb (uv__run_prepare)"); uv_close((uv_handle_t*)h, NULL); }
static void check_cb(uv_check_t* h) { HIT("check_cb (uv__run_check)"); uv_close((uv_handle_t*)h, NULL); }
static void walk_cb(uv_handle_t* h, void* arg) { (*(int*)arg)++; }
int addon_watchers(uv_loop_t* l) {
  static int n;
  uv_idle_init(l, &idle_h); uv_idle_start(&idle_h, idle_cb);
  uv_prepare_init(l, &prep_h); uv_prepare_start(&prep_h, prep_cb);
  uv_check_init(l, &check_h); uv_check_start(&check_h, check_cb);
  uv_walk(l, walk_cb, &n); HIT("walk_cb (uv_walk)");
  return 0;
}

/* pipe: read/write/shutdown/eof */
static uv_pipe_t p0, p1; static uv_write_t wreq; static uv_shutdown_t sreq; static char rbuf[64];
static void alloc_cb(uv_handle_t* h, size_t s, uv_buf_t* b) { HIT("alloc_cb (uv__read)"); *b = uv_buf_init(rbuf, sizeof rbuf); }
static void read_cb(uv_stream_t* s, ssize_t n, const uv_buf_t* b) {
  if (n > 0) HIT("read_cb data (uv__read)");
  else if (n == UV_EOF) { HIT("read_cb EOF (uv__stream_eof)"); uv_close((uv_handle_t*)s, close_cb); }
}
static void write_cb(uv_write_t* r, int s) { HIT("write_cb (uv__write_callbacks)"); }
static void shutdown_cb(uv_shutdown_t* r, int s) { HIT("shutdown_cb (uv__drain)"); uv_close((uv_handle_t*)r->handle, NULL); }
int addon_pipe(uv_loop_t* l) {
  uv_file fds[2]; uv_buf_t b = uv_buf_init("hello", 5);
  if (uv_pipe(fds, 0, 0)) return -1;
  uv_pipe_init(l, &p0, 0); uv_pipe_init(l, &p1, 0);
  uv_pipe_open(&p0, fds[0]); uv_pipe_open(&p1, fds[1]);
  uv_read_start((uv_stream_t*)&p0, alloc_cb, read_cb);
  uv_write(&wreq, (uv_stream_t*)&p1, &b, 1, write_cb);
  return uv_shutdown(&sreq, (uv_stream_t*)&p1, shutdown_cb);
}

/* tcp: listen/connect */
static uv_tcp_t srv, cli, acc; static uv_connect_t creq;
static void conn_cb(uv_connect_t* r, int s) { HIT("connect_cb (uv__stream_connect)"); uv_close((uv_handle_t*)&cli, NULL); }
static void connection_cb(uv_stream_t* s, int st) {
  HIT("connection_cb (uv__server_io)");
  uv_tcp_init(s->loop, &acc); uv_accept(s, (uv_stream_t*)&acc);
  uv_close((uv_handle_t*)&acc, NULL); uv_close((uv_handle_t*)s, NULL);
}
int addon_tcp(uv_loop_t* l) {
  struct sockaddr_in a; struct sockaddr_storage n; int len = sizeof n;
  uv_ip4_addr("127.0.0.1", 0, &a);
  uv_tcp_init(l, &srv); uv_tcp_bind(&srv, (struct sockaddr*)&a, 0);
  if (uv_listen((uv_stream_t*)&srv, 1, connection_cb)) return -1;
  uv_tcp_getsockname(&srv, (struct sockaddr*)&n, &len);
  uv_tcp_init(l, &cli);
  return uv_tcp_connect(&creq, &cli, (struct sockaddr*)&n, conn_cb);
}

/* udp */
static uv_udp_t us, ur; static uv_udp_send_t ureq; static char ubuf[64];
static void ualloc(uv_handle_t* h, size_t s, uv_buf_t* b) { *b = uv_buf_init(ubuf, sizeof ubuf); }
static void urecv(uv_udp_t* h, ssize_t n, const uv_buf_t* b, const struct sockaddr* a, unsigned f) {
  if (n > 0) { HIT("udp recv_cb (uv__udp_recvmsg/recvmmsg)"); uv_close((uv_handle_t*)h, NULL); }
}
static void usend(uv_udp_send_t* r, int s) { HIT("udp send_cb (uv__udp_run_completed)"); uv_close((uv_handle_t*)r->handle, NULL); }
int addon_udp(uv_loop_t* l) {
  struct sockaddr_in a; struct sockaddr_storage n; int len = sizeof n; uv_buf_t b = uv_buf_init("x", 1);
  uv_ip4_addr("127.0.0.1", 0, &a);
  uv_udp_init(l, &ur); uv_udp_bind(&ur, (struct sockaddr*)&a, 0); uv_udp_getsockname(&ur, (struct sockaddr*)&n, &len);
  uv_udp_recv_start(&ur, ualloc, urecv);
  uv_udp_init(l, &us);
  return uv_udp_send(&ureq, &us, &b, 1, (struct sockaddr*)&n, usend);
}

/* fs + fs_poll + random + getaddrinfo */
static uv_fs_t freq; static uv_fs_poll_t fsp; static uv_random_t rreq; static char rnd[8]; static uv_getaddrinfo_t gai;
static void fs_cb(uv_fs_t* r) { HIT("fs_cb (uv__fs_done)"); uv_fs_req_cleanup(r); }
static void fsp_cb(uv_fs_poll_t* h, int st, const uv_stat_t* a, const uv_stat_t* b) { HIT("fs_poll_cb (poll_cb)"); uv_close((uv_handle_t*)h, NULL); }
static void rnd_cb(uv_random_t* r, int s, void* b, size_t n) { HIT("random_cb (uv__random_done)"); }
static void gai_cb(uv_getaddrinfo_t* r, int s, struct addrinfo* ai) { HIT("getaddrinfo_cb (uv__getaddrinfo_done)"); uv_freeaddrinfo(ai); }
int addon_fs(uv_loop_t* l) {
  struct addrinfo hints; memset(&hints, 0, sizeof hints); hints.ai_flags = AI_NUMERICHOST;
  uv_fs_stat(l, &freq, "/", fs_cb);
  uv_fs_poll_init(l, &fsp); uv_fs_poll_start(&fsp, fsp_cb, "/nonexistent-cfi-test", 10);
  uv_random(l, &rreq, rnd, sizeof rnd, 0, rnd_cb);
  return uv_getaddrinfo(l, &gai, gai_cb, "127.0.0.1", NULL, &hints);
}

/* signal + process + poll + fs_event */
static uv_signal_t sig; static uv_process_t proc; static uv_poll_t pl; static uv_fs_event_t fse; static int pfd[2];
static char dir[] = "/tmp/cfi-fse-XXXXXX";
static void sig_cb(uv_signal_t* h, int s) { HIT("signal_cb (uv__signal_event)"); uv_close((uv_handle_t*)h, NULL); }
static void exit_cb(uv_process_t* p, int64_t st, int t) { HIT("exit_cb (uv__wait_children)"); uv_close((uv_handle_t*)p, NULL); }
static void poll_cb(uv_poll_t* h, int st, int ev) { HIT("poll_cb (uv__poll_io)"); uv_close((uv_handle_t*)h, NULL); }
static void fse_cb(uv_fs_event_t* h, const char* f, int ev, int st) { HIT("fs_event_cb (uv__inotify_read)"); uv_close((uv_handle_t*)h, NULL); }
int addon_os(uv_loop_t* l) {
  char* args[] = {"/bin/true", NULL}; uv_process_options_t o; char path[64];
  memset(&o, 0, sizeof o); o.file = args[0]; o.args = args; o.exit_cb = exit_cb;
  uv_signal_init(l, &sig); uv_signal_start_oneshot(&sig, sig_cb, SIGUSR1);
  if (uv_spawn(l, &proc, &o)) HIT("spawn failed");
  if (pipe(pfd) == 0) { write(pfd[1], "x", 1); uv_poll_init(l, &pl, pfd[0]); uv_poll_start(&pl, UV_READABLE, poll_cb); }
  if (mkdtemp(dir)) {
    uv_fs_event_init(l, &fse); uv_fs_event_start(&fse, fse_cb, dir, 0);
    snprintf(path, sizeof path, "%s/f", dir); close(open(path, O_CREAT | O_WRONLY, 0600));
  }
  raise(SIGUSR1);
  return 0;
}

/* addon tries to install its own allocator: contract role 'runtime' => must trap */
static void* a_malloc(size_t n) { return malloc(n); }
static void* a_realloc(void* p, size_t n) { return realloc(p, n); }
static void* a_calloc(size_t a, size_t b) { return calloc(a, b); }
static void a_free(void* p) { free(p); }
int addon_allocator(uv_loop_t* l) { return uv_replace_allocator(a_malloc, a_realloc, a_calloc, a_free); }
