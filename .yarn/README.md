# Vendored Yarn release

This directory holds the Yarn release used by this repo (`yarnPath` in
`.yarnrc.yml`). The release file is checked in so every contributor and CI job
runs the exact same Yarn, and so we can carry small local patches when needed.

`releases/yarn-4.12.0.cjs` currently carries one such patch, described below.
If you bump the Yarn version, read the **Upgrading Yarn** section first.

## Patch: use `JsZipImpl` for the node-modules link step

### What changed

Two call sites in `releases/yarn-4.12.0.cjs` are modified so the
`node-modules` linker (and the `pnpm`-loose linker) construct their read-only
`ZipOpenFS` with `customZipImplementation: ST` — Yarn's pure-JS `JsZipImpl` —
instead of falling through to the default WASM-backed `LibZipImpl`:

```text
new $f({maxOpenFiles:80,readOnlyArchives:!0})
  → new $f({maxOpenFiles:80,readOnlyArchives:!0,customZipImplementation:ST})
```

A comment block at the top of the `.cjs` file marks the file as patched and
points back here.

### Why

On the `linux-arm` CI test shards we run a 32-bit `arm32v7` container. During
`yarn install`'s **Link step**, Yarn opens up to 80 cache zips concurrently.
With `LibZipImpl`, each open zip is `readFileSync`'d into a Node `Buffer`
**and copied again into the WASM linear memory**, and every file read does a
WASM `_malloc(size)` for the entry. The WASM heap has to grow as a single
contiguous region of the 32-bit address space; once enough zips are resident,
the `_malloc` for a large entry — most often `typescript/lib/typescript.js`
(~9 MB inside a ~22 MB zip) — fails.

Yarn's cross-FS `copyFilePromise` swallows the underlying error and re-throws
a generic one, so CI shows:

```text
YN0001: While persisting .../typescript-patch-...zip/node_modules/typescript/
  EINVAL: invalid argument, copyfile '/node_modules/typescript/lib/typescript.js' -> '...'
```

The unmasked form (occasionally seen on `pdfjs-dist`) is the WASM-heap failure
string `Couldn't allocate enough memory`. This started failing ~1-in-3
`linux-arm / test` shards at **Install Dependencies** on 2026-04-13, after
[#50692](https://github.com/electron/electron/pull/50692) grew the cache enough
to push the 32-bit process over the edge nondeterministically — e.g.
[run 24739817558](https://github.com/electron/electron/actions/runs/24739817558/job/72380803746).

`JsZipImpl` avoids the problem entirely: it opens the zip by file descriptor,
reads only the central directory into memory, and `readSync`s individual
entries into ordinary Node `Buffer`s — **no WASM heap involved**. It is
read-only and path-based, which is exactly how the linker uses these archives.

There is no `.yarnrc.yml` setting or environment variable to select the zip
implementation (verified against the bundle), so editing the vendored release
is the only way to switch it short of re-implementing the linker in a plugin.

Upstream references:
[yarnpkg/berry#3972](https://github.com/yarnpkg/berry/issues/3972),
[yarnpkg/berry#6722](https://github.com/yarnpkg/berry/issues/6722),
[yarnpkg/berry#6550](https://github.com/yarnpkg/berry/issues/6550).

### Upgrading Yarn

When bumping `releases/yarn-*.cjs`:

1. Check whether upstream now defaults `readOnlyArchives` opens to `JsZipImpl`,
   or exposes a config knob for the zip implementation. If so, drop this patch.
2. Otherwise, re-apply: search the new bundle for
   `maxOpenFiles:80,readOnlyArchives:!0` (the surrounding minified identifiers
   will differ) and add `,customZipImplementation:<JsZipImpl symbol>` — that
   symbol is whatever the new bundle exports as `JsZipImpl` from
   `@yarnpkg/libzip`.
3. Re-add the header comment pointing back to this README.
4. Verify with
   `rm -rf node_modules spec/node_modules && node script/yarn.js install --immutable --mode=skip-build`
   and confirm `node_modules/typescript/lib/typescript.js` is byte-identical to
   an unpatched install.
