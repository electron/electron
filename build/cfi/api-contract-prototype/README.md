# API-rooted CFI callback analysis (libuv prototype)

A prototype of the API-rooted alternative to the IR call-site inventory
(`ir_callback_manifest.txt`) used in this PR. Instead of listing every indirect
call in V8/libuv, it starts from the **public API**:

1. **Contract.** Discover every callback-bearing declaration in libuv's public
   headers: function-pointer params, callback typedefs and callback fields.
   `tools/contract.txt` is a small reviewed file that gives each one a role
   (`addon`, `runtime`, `addon-input`, `n/a`). The analysis **fails** on any
   public callback declaration that is not in the contract, and on any contract
   entry that no longer matches a declaration.
2. **Provenance.** Trace addon-supplied values from API params through
   assignments, struct fields, locals, function params (interprocedural),
   returns, casts and unions to the indirect calls that invoke them. Fields are
   keyed by declaration rather than by object, so `container_of`, `uv__queue`
   and handle-type casts need no special models. Every indirect call is
   classified `addon`, `internal` or `UNRESOLVED`. UNRESOLVED calls block until
   a reviewed `site:` contract entry covers them.
3. **Policy.** Generate a narrow `[cfi-icall]` ignorelist (`fun:` entries for
   the functions that contain addon call sites), meant to be passed **only to
   the libuv target's cflags**. A function that would also exempt an internal
   indirect call is reported as a **mixed dispatcher** and fails.
4. **Verify.** Check the frontend IR per function: exempt functions emit no
   `llvm.type.test`, and checked functions keep one per call site. The type ids
   at internal call sites must match the targets' `!type` ids. Then run
   runtime tests with an uninstrumented addon `.so`.

The analysis needs only the AST (no inlining or optimization), so it can run as
an ordinary action in the same testing or release build, before the libuv
target compiles.

## Requirements

- clang and lld 18 or newer (`clang`, `ld.lld`), with ThinLTO and CFI support
- python3 (stdlib only; the scripts parse `clang -Xclang -ast-dump=json`)
- git, to fetch libuv

## Running

All paths are relative to this directory. libuv is fetched to `.libuv/` and
outputs go to `out/`; both are gitignored. Override them with `LIBUV_DIR` and
`OUT_DIR`.

```bash
cd build/cfi/api-contract-prototype

# 1. libuv at the version Electron's Node ships (Node v24.22.1 -> libuv v1.52.1)
tools/fetch_libuv.sh                       # -> .libuv/

# 2. List the public callback surface (what the contract must cover)
python3 -I tools/analyze.py --root .libuv --contract tools/contract.txt --out out/disc --discover

# 3. Extract + analyze + generate the policy (exit 1 on any error)
python3 -I tools/analyze.py --root .libuv --contract tools/contract.txt --out out/base
#    -> out/base/report.txt, out/base/ignorelist.generated.txt, out/base/sites.json

# 4. Verify the emitted checks per function in the frontend IR
tools/emit_ir.sh .libuv out/base/ignorelist.generated.txt out/base/ir
python3 -I tools/verify_ir.py out/base/sites.json out/base/ignorelist.generated.txt out/base/ir/*.ll

# 5. Mutation tests (new API, moved call, renamed field, cast into a runtime
#    field, opaque memcpy, mixed dispatcher)
python3 -I tools/mutate.py .libuv tools/contract.txt out

# 6. Runtime tests: CFI-instrumented ThinLTO host + uninstrumented addon .so.
#    The optional 2nd arg is an electron checkout at this PR, to also test the PR's list.
rt/run_matrix.sh out/base/ignorelist.generated.txt ../../..
```

`results/` holds the output of these steps from the original run.

## Results (libuv v1.52.1, Linux x64)

- 134 public callback declarations were discovered (fields, params and
  typedefs); the contract has 137 entries, including 3 reviewed `site:` entries.
- 58 indirect call sites: 45 addon, 10 internal, 3 external (reviewed).
- The analysis generates **33** `fun:` entries in about 7 s wall time on 4
  cores (about 24 s on one). That breaks down as ~7 s of clang JSON dumps and
  ~16 s of Python parsing; the global solve takes under 0.1 s.
- It reproduces **all 30** of the PR's `fun:uv*` entries. The PR's
  `uv__fs_poll_cb` is upstream's `poll_cb`, which Node's patch renames; a list
  scoped to the libuv target makes that rename unnecessary. It finds no entry
  the PR has that the analysis lacks.
- The analysis found 3 call sites that the PR keeps checked:
  - `uv__preadv_or_pwritev`: `dlsym`'d `preadv64`/`pwritev64`. The libc target
    is outside the CFI jump tables, so the checked call traps.
  - `uv__fs_mkstemp`: `dlsym`'d `mkostemp`. Same problem.
  - `uv__getsockpeername`: `getsockname`/`getpeername` called through a
    pointer. With clang 18, glibc's `__SOCKADDR_ARG` transparent union gives a
    different CFI type id. This does **not** reproduce with Chromium's clang
    (see below).
- The per-function check counts match the PR manifest exactly (e.g. `uv__read`
  has 6, `uv__udp_recvmsg` 5, `heap_remove` 3).
- Rebuild scope: clang records the ignorelist in every depfile. The PR's
  global `cfi_flags` list makes any edit a full rebuild. Scoped to the libuv
  target, an edit rebuilds 35 TUs plus the relink.

### Acceptance criteria

| | Criterion | Result |
|---|---|---|
| a | Discover the work/completion addon dispatchers from public API roots | PASS: all 30 PR entries plus 3 more, each with a provenance path |
| b | Keep internal dispatch internal and checked | PASS. `uv__io_t.cb` no longer exists in 1.52 (enum + switch), so the equivalent `uv__work.work/done` in public `uv/threadpool.h` was used: `worker` and `uv__work_done` stay checked, as do heap `less_than` and the allocator |
| c | Detect an added callback API, a moved invocation, a changed field | PASS: uncontracted decls fail; a moved call updates the policy automatically; a renamed field reports stale + uncontracted entries; a cast into the runtime field reports a contract violation |
| d | Report opaque/ambiguous flows | PASS: `dlsym` / atomic-cache sites and a `memcpy`'d callback are UNRESOLVED and block; union punning is modeled |
| e | Policy does not exempt unrelated calls | PASS: 0 mixed dispatchers in 1.52.1 (the mutation is detected); the IR check confirms |
| f | Runtime tests + negative controls | PASS: 15 scenarios (28/33 exempt dispatchers exercised) pass with the generated list. Removing `fun:uv__run_timers` gives SIGILL. An empty list traps every addon scenario. A wrongly typed internal allocator traps at the protected `uv__malloc`. An addon calling `uv_replace_allocator` (role `runtime`) traps, which is fail-closed by contract |

**Mixed-provenance exempt sites (8).** These call sites also dispatch internal
callbacks, which lose their check when the site is exempted:
- `uv__async_io` also dispatches `uv__work_done`.
- `uv__signal_event` also dispatches `uv__chld`.
- `uv__finish_close`, `uv__run_timers`, `uv__fs_done` and `uv__poll_io_uring`
  also dispatch fs-poll's internal callbacks.
- `uv__preadv_or_pwritev` also dispatches the `*_emul` fallbacks.
- `uv__getsockpeername` (libc targets).

Splitting these would need code changes, for example comparing against the
internal function and calling it directly.

### Verified on a release CFI build of #54561 (Chromium's clang)

- `fs.readvSync(fd, [b1, b2], 0)` and `fs.readv(fd, [b1, b2], 0, cb)`:
  **SIGILL** (`uv__preadv_or_pwritev`).
- `NODE_COMPILE_CACHE=<dir> node a.js`: **SIGILL**. This is `uv_fs_mkstemp` in
  `src/compile_cache.cc` reaching `uv__fs_mkstemp`.
- `server.address()` and `socket.remoteAddress`/`remotePort`: **no trap**.
  The `uv__getsockpeername` type mismatch only shows up with clang 18 and is
  not an issue for Electron's toolchain. Its `site:` entry in the contract is
  only needed for the clang 18 runtime test here.

## Limits

- This is a prototype that covers **libuv only** (C, Linux x64 sources and
  defines from Node's `uv.gyp`).
- The analysis is flow-insensitive and keyed by field rather than by object,
  which is conservative.
- An internal provenance does not guarantee the check passes (dlsym'd or
  prototype-mismatched targets). The IR type-id check and the runtime tests are
  part of the method, not optional extras.
- Node and V8 would need:
  - a libTooling or clang-plugin fact extractor, since JSON ASTs are too large
    for C++ TUs;
  - models for lambdas, templates, `std::function`, member function pointers
    and containers;
  - models for values stored in V8's tagged heap (`Address` slots,
    Foreign/Managed, external-pointer accessors; `ExternalPointerTag` is a
    natural key);
  - `[cfi-vcall] type:` entries for embedder-implemented virtual interfaces.

  For Node, keeping the `CallAddonEntryPoint` trampolines and having the
  analysis check that every addon call site is inside one avoids brittle
  mangled `fun:` names.

## Files

- `tools/extract.py`: per-TU facts from the clang JSON AST
- `tools/analyze.py`: contract check, provenance solve, classification, policy and report
- `tools/contract.txt`: the reviewed libuv public callback contract
- `tools/verify_ir.py`, `tools/emit_ir.sh`: per-function check of the emitted CFI checks and type ids
- `tools/mutate.py`: mutation tests
- `tools/fetch_libuv.sh`: fetches libuv
- `rt/host.c`, `rt/addon.c`, `rt/build.sh`, `rt/run_matrix.sh`: runtime tests
- `results/`: outputs from the original run
