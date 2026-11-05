"""Mutation tests: apply a source edit to a copy of libuv, rerun the analysis, show what changed.

usage: python3 -I mutate.py <libuv root> <contract> <outdir>
"""
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

MUTATIONS = {
    # (c-i) a new public API taking a callback, invoked from a new internal function
    "new_api": [
        ("include/uv.h", "UV_EXTERN int uv_timer_start(uv_timer_t* handle,",
         "UV_EXTERN int uv_timer_on_fire(uv_timer_t* handle, uv_timer_fire_cb cb);\n"
         "UV_EXTERN int uv_timer_start(uv_timer_t* handle,"),
        ("include/uv.h", "typedef void (*uv_timer_cb)(uv_timer_t* handle);",
         "typedef void (*uv_timer_cb)(uv_timer_t* handle);\n"
         "typedef void (*uv_timer_fire_cb)(uv_timer_t* handle, int late);"),
        ("include/uv/unix.h", "  uv_timer_cb timer_cb;                                                       \\",
         "  uv_timer_cb timer_cb;                                                       \\\n"
         "  uv_timer_fire_cb fire_cb;                                                   \\"),
        ("src/timer.c", "void uv__run_timers(uv_loop_t* loop) {",
         "int uv_timer_on_fire(uv_timer_t* handle, uv_timer_fire_cb cb) {\n"
         "  handle->fire_cb = cb;\n  return 0;\n}\n\n"
         "static void uv__timer_notify(uv_timer_t* handle) {\n"
         "  if (handle->fire_cb != NULL)\n    handle->fire_cb(handle, 0);\n}\n\n"
         "void uv__run_timers(uv_loop_t* loop) {"),
        ("src/timer.c", "    handle->timer_cb(handle);\n  }\n}",
         "    handle->timer_cb(handle);\n    uv__timer_notify(handle);\n  }\n}"),
    ],
    # (c-ii) an invocation moves into a helper
    "moved_call": [
        ("src/threadpool.c",
         "static void uv__queue_work(struct uv__work* w) {\n  uv_work_t* req = container_of(w, uv_work_t, work_req);\n\n  req->work_cb(req);\n}",
         "static void uv__invoke_work(uv_work_t* req) {\n  req->work_cb(req);\n}\n\n"
         "static void uv__queue_work(struct uv__work* w) {\n  uv_work_t* req = container_of(w, uv_work_t, work_req);\n\n  uv__invoke_work(req);\n}"),
    ],
    # (c-iii) a callback field is renamed
    "renamed_field": [
        ("include/uv.h", "  uv_work_cb work_cb;", "  uv_work_cb work_fn;"),
        ("src/threadpool.c", "req->work_cb(req);", "req->work_fn(req);"),
        ("src/threadpool.c", "req->work_cb = work_cb;", "req->work_fn = work_cb;"),
    ],
    # (c-iv) a callback field changes: the addon callback is laundered through a cast into the
    # runtime-only uv__work.work field (provenance must survive the cast)
    "cast_into_runtime_field": [
        ("src/threadpool.c", "  req->work_cb = work_cb;",
         "  req->work_cb = work_cb;\n  req->work_req.work = (void (*)(struct uv__work*)) work_cb;"),
    ],
    # (d) opaque flow: the callback is copied with memcpy, so provenance is lost
    "opaque_memcpy": [
        ("src/threadpool.c", "  req->work_cb = work_cb;", "  memcpy(&req->work_cb, &work_cb, sizeof(work_cb));"),
    ],
    # (e) a mixed dispatcher: an internal indirect call added to an exempt function
    "mixed_dispatcher": [
        ("src/timer.c", "    handle->timer_cb(handle);\n  }\n}",
         "    handle->timer_cb(handle);\n  }\n  {\n    void (*hook)(uv_loop_t*) = uv__timer_trace;\n    hook(loop);\n  }\n}"),
        ("src/timer.c", "void uv__run_timers(uv_loop_t* loop) {",
         "static void uv__timer_trace(uv_loop_t* loop) { (void) loop; }\n\nvoid uv__run_timers(uv_loop_t* loop) {"),
    ],
}


def main():
    root, contract, outdir = sys.argv[1:4]
    base_pol = os.path.join(outdir, "base", "ignorelist.generated.txt")
    subprocess.run([sys.executable, "-I", os.path.join(HERE, "analyze.py"), "--root", root,
                    "--contract", contract, "--out", os.path.join(outdir, "base")],
                   capture_output=True)
    base = {l.strip() for l in open(base_pol) if l.startswith("fun:")}
    for name, edits in MUTATIONS.items():
        tree = os.path.join(outdir, "mut", name, "libuv")
        if os.path.exists(tree):
            shutil.rmtree(tree)
        shutil.copytree(root, tree, ignore=shutil.ignore_patterns(".git", "test", "docs"))
        for path, old, new in edits:
            p = os.path.join(tree, path)
            s = open(p).read()
            if old not in s:
                raise SystemExit("%s: anchor not found in %s" % (name, path))
            open(p, "w").write(s.replace(old, new, 1))
        od = os.path.join(outdir, "mut", name)
        r = subprocess.run([sys.executable, "-I", os.path.join(HERE, "analyze.py"), "--root", tree,
                            "--contract", contract, "--out", od], capture_output=True, text=True)
        rep = open(os.path.join(od, "report.txt")).read()
        errs = rep.split("== ERRORS")[1].split("== WARNINGS")[0].strip().split("\n")[1:]
        warns = rep.split("== WARNINGS")[1].split("== MIXED")[0].strip().split("\n")[1:]
        pol = {l.strip() for l in open(os.path.join(od, "ignorelist.generated.txt")) if l.startswith("fun:")}
        print("=== %s: analyzer exit %d" % (name, r.returncode))
        for e in errs:
            print("  E " + e.strip()[:260])
        for w in warns:
            if "pthread" not in w:
                print("  W " + w.strip()[:260])
        print("  policy +%s -%s" % (sorted(pol - base), sorted(base - pol)))


if __name__ == "__main__":
    main()
