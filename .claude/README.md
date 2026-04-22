# `.claude/`

This directory holds Claude Code project configuration (skills, settings) for
the Electron repo. It also documents local patches that were investigated or
applied with Claude Code so future contributors can find the rationale.

## Yarn JsZipImpl patch (`.yarn/releases/yarn-4.12.0.cjs`)

### What is patched

Two call sites in the vendored Yarn 4.12.0 bundle are modified so the
`node-modules` linker (and the `pnpm`-loose linker) construct their
`ZipOpenFS` with `customZipImplementation: ST` (Yarn's pure-JS `JsZipImpl`)
instead of falling through to the default `LibZipImpl` (Emscripten/WASM
libzip):

```text
new $f({maxOpenFiles:80,readOnlyArchives:!0})
  → new $f({maxOpenFiles:80,readOnlyArchives:!0,customZipImplementation:ST})
```

A comment block at the top of the `.cjs` file marks the patch.

### Why

On the `linux-arm` test shards we run a 32-bit `arm32v7` container. During
`yarn install`'s **Link step**, Yarn opens up to 80 cache zips concurrently.
With `LibZipImpl`, each open zip is `readFileSync`'d into a Node `Buffer` and
**copied again into the WASM linear memory**, and every file read does a WASM
`_malloc(size)` for the entry. The WASM heap must grow in a single contiguous
region of the 32-bit address space, and once enough zips are resident the
`_malloc` for a large entry (notably `typescript/lib/typescript.js`, ~9 MB
inside a ~22 MB zip) fails.

Yarn's cross-FS `copyFilePromise` swallows the underlying error and re-throws
a generic one, so CI shows:

```text
YN0001: While persisting .../typescript-patch-...zip/node_modules/typescript/
  EINVAL: invalid argument, copyfile '/node_modules/typescript/lib/typescript.js' -> '...'
```

The unmasked form (seen on `pdfjs-dist`) is the WASM-heap failure string
`Couldn't allocate enough memory`. This caused ~1-in-3 `linux-arm / test`
shards to fail at **Install Dependencies** starting 2026-04-13, after
[#50692](https://github.com/electron/electron/pull/50692) grew the cache enough
to push the 32-bit process over the edge nondeterministically. See e.g.
[run 24739817558](https://github.com/electron/electron/actions/runs/24739817558/job/72380803746).

`JsZipImpl` avoids the problem entirely: it opens the zip by file descriptor,
reads only the central directory into memory, and `readSync`s individual
entries into ordinary Node `Buffer`s — **no WASM heap at all**. It is
read-only and path-based, which is exactly how the linker uses it.

There is no `.yarnrc.yml` setting or env var to select the zip implementation
(verified against the bundle), so editing the vendored release is the only way
to switch it without re-implementing the linker in a plugin.

Upstream references:
[yarnpkg/berry#3972](https://github.com/yarnpkg/berry/issues/3972),
[yarnpkg/berry#6722](https://github.com/yarnpkg/berry/issues/6722),
[yarnpkg/berry#6550](https://github.com/yarnpkg/berry/issues/6550).

### When upgrading Yarn

If you bump `.yarn/releases/yarn-*.cjs`:

1. Check whether upstream now defaults `readOnlyArchives` opens to `JsZipImpl`,
   or exposes a config knob for it. If so, drop this patch.
2. Otherwise, re-apply: search the new bundle for
   `maxOpenFiles:80,readOnlyArchives:!0` (the minified identifiers around it
   will differ) and add `,customZipImplementation:<JsZipImpl symbol>` — the
   symbol is whatever the bundle exports as `JsZipImpl` from `@yarnpkg/libzip`.
3. Verify with `rm -rf node_modules spec/node_modules && node script/yarn.js
   install --immutable --mode=skip-build` and confirm
   `node_modules/typescript/lib/typescript.js` is byte-identical to an
   unpatched install.
