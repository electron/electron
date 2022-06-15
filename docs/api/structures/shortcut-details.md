# ShortcutDetails Object

* `target` string - The target to launch from this shortcut.
* `cwd` string (optional) - The working directory. Default is empty.
* `args` string (optional) - The arguments to be applied to `target` when
launching from this shortcut. Default is empty.
* `description` string (optional) - The description of the shortcut. Default
is empty.
* `icon` string (optional) - The path to the icon, can be a DLL or EXE. `icon`
and `iconIndex` have to be set together. Default is empty, which uses the
target's icon.
* `iconIndex` number (optional) - The resource ID of icon when `icon` is a
DLL or EXE. Default is 0.
* `appUserModelId` string (optional) - The Application User Model ID. Default
is empty.
* `toastActivatorClsid` string (optional) - The Application Toast Activator CLSID. Needed
for participating in Action Center.
