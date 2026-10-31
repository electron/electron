# Verifying the accessibility hit-test of an Electron window (Windows)

This folder describes how to verify, on Windows, that the mouse pointer does
not land on an empty "pane" instead of the web content of the window.

## Why two paths are measured

A single point on the screen can be interpreted in two ways by Windows:

- **UIA** (UI Automation): `AutomationElement.FromPoint`.
- **MSAA** (IAccessible): `AccessibleObjectFromPoint`, in `oleacc.dll`.

The second one is the legacy path, the one NVDA uses in its legacy mode for
Electron applications, and the one Inspect.exe shows in MSAA mode. That is the
path that was blocked by the empty `contentView` placed above the
`WebContentsView` (electron/electron#42945).

## Files

- `app.js`: test application. It opens a window, places texts at fixed
  positions, and writes a measurements file in the temporary directory. By
  default the `contentView` stays empty, like in an ordinary `BrowserWindow`;
  the `--avec-enfant` option adds a 40 x 40 view to it.
- `uia_check.ps1`: checker. It queries both paths at the control points and
  writes the result as plain text, one line per result.

## Steps

1. Build Electron, or use a published version.
2. Run the test application with the binary to check:

   ```
   <electron.exe> <folder of this file>
   ```

   Add `--avec-enfant` for the second verification case.
3. From another window, query the control points:

   ```
   powershell.exe -ExecutionPolicy Bypass -File uia_check.ps1 -Json %TEMP%\electron-a11y-hit-test.json
   ```

The measurements file contains the position of the window content area, the
screen scale factor and the position of the control points, in logical
pixels. The checker applies the scale factor, because the accessibility API
works in physical pixels.

## Expected result

The `ATTEINT` line of the MSAA path gives the role, the name and the number
of accessible children of the object returned by `AccessibleObjectFromPoint`.
The number of children is what tells the two situations apart: the empty
"pane" has none, the node that carries the web content has one.

### First case: the `contentView` is empty (default configuration)

- **Before the fix**: the MSAA path returns, at every text point, a
  `role=16` (`ROLE_SYSTEM_PANE`) object, with no name and **no accessible
  child**. That is the empty "pane": a screen reader on the legacy path has
  nothing to say. The accessibility tree contains, next to the web document,
  an extra empty container.
- **After the fix**: the empty "pane" is gone from the accessibility tree,
  and the MSAA path returns the node that carries the web document:
  `role=16` with **one** accessible child, the document.

### Second case: the `contentView` holds a view (`--avec-enfant`)

The `contentView` is not empty any more, so it has to stay in the
accessibility tree.

- **Before the fix**: the MSAA path returns the `contentView` itself at every
  point, including the ones that are over the web content.
- **After the fix**: the `contentView` keeps the hit-test where one of its
  children covers the pointer, and the hit-test goes through to the web
  content elsewhere.

The objects returned by the MSAA path are the same before and after the fix
in that configuration: the `contentView` and the node that carries the web
content have the same bounds and the same number of accessible children, so
the two cannot be told apart from the returned object alone. The first case is
the discriminating one.

### UIA path

The UIA path returns the web document in both cases, before and after the
fix: it was not affected by the defect. Only the empty container disappears
from the tree.