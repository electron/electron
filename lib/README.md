# `/lib`

JavaScript source code, structured like the C++ files in the [`atom`](../atom) directory.

Includes custom APIs written to support DevTools extensions in Electron contexts, since that isn't included in the content module.

By default, these directories are not available to users, but the files in the `exports` are:

- [browser/api/exports](browser/api/exports)
- [common/api/exports](common/api/exports)
- [renderer/api/exports](renderer/api/exports)
