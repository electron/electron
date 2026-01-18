# Debugging in VSCode

This guide goes over how to set up VSCode debugging for both your own Electron
project as well as the native Electron codebase.

## Debugging your Electron app

### Main process

#### 1. Open an Electron project in VSCode.

```sh
$ npx create-electron-app@latest my-app
$ code my-app
```

#### 2. Add a file `.vscode/launch.json` with the following configuration:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug Main Process",
      "type": "node",
      "request": "launch",
      "cwd": "${workspaceFolder}",
      "runtimeExecutable": "${workspaceFolder}/node_modules/.bin/electron",
      "windows": {
        "runtimeExecutable": "${workspaceFolder}/node_modules/.bin/electron.cmd"
      },
      "args": ["."],
      "outputCapture": "std"
    }
  ]
}
```

#### 3. Debugging

Set some breakpoints in `main.js`, and start debugging in the
[Debug View](https://code.visualstudio.com/docs/editor/debugging). You should
be able to hit the breakpoints.

### Preload scripts

Debugging preload scripts requires a slightly different configuration as they run in the renderer process but are initialized before the web page.

#### 1. Configure `webPreferences`

Ensure your `BrowserWindow` configuration allows for debugging. Sometimes `sandbox: false` is required to allow the debugger to attach reliably to the preload script, although this affects the security model of your application.

```js
const win = new BrowserWindow({
  webPreferences: {
    preload: path.join(__dirname, "preload.js"),
    sandbox: false, // May be required for debugging preload scripts
  },
});
```

#### 2. Update `.vscode/launch.json`

Add a configuration to attach to the renderer process. Note the `runtimeArgs` adding the remote debugging port.

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug Main Process",
      "type": "node",
      "request": "launch",
      "cwd": "${workspaceFolder}",
      "runtimeExecutable": "${workspaceFolder}/node_modules/.bin/electron",
      "windows": {
        "runtimeExecutable": "${workspaceFolder}/node_modules/.bin/electron.cmd"
      },
      "args": ["--remote-debugging-port=9223", "."],
      "outputCapture": "std"
    },
    {
      "name": "Debug Preload/Renderer",
      "type": "chrome",
      "request": "attach",
      "port": 9223,
      "webRoot": "${workspaceFolder}",
      "timeout": 30000
    }
  ],
  "compounds": [
    {
      "name": "Debug All",
      "configurations": ["Debug Main Process", "Debug Preload/Renderer"]
    }
  ]
}
```

#### 3. Debugging

1. Select "Debug All" from the debug dropdown.
2. Set breakpoints in your `preload.js`.
3. Start debugging. The debugger should attach to both the main process and the renderer (where the preload script runs), allowing you to hit breakpoints in the preload script.

## Debugging the Electron codebase

If you want to build Electron from source and modify the native Electron codebase,
this section will help you in testing your modifications.

For those unsure where to acquire this code or how to build it,
[Electron's Build Tools](https://github.com/electron/build-tools) automates and
explains most of this process. If you wish to manually set up the environment,
you can instead use these [build instructions](../development/build-instructions-gn.md).

### Windows (C++)

#### 1. Open an Electron project in VSCode.

```sh
$ npx create-electron-app@latest my-app
$ code my-app
```

#### 2. Add a file `.vscode/launch.json` with the following configuration:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "(Windows) Launch",
      "type": "cppvsdbg",
      "request": "launch",
      "program": "${workspaceFolder}\\out\\your-executable-location\\electron.exe",
      "args": ["your-electron-project-path"],
      "stopAtEntry": false,
      "cwd": "${workspaceFolder}",
      "environment": [
        { "name": "ELECTRON_ENABLE_LOGGING", "value": "true" },
        { "name": "ELECTRON_ENABLE_STACK_DUMPING", "value": "true" },
        { "name": "ELECTRON_RUN_AS_NODE", "value": "" }
      ],
      "externalConsole": false,
      "sourceFileMap": {
        "o:\\": "${workspaceFolder}"
      }
    }
  ]
}
```

**Configuration Notes**

- `cppvsdbg` requires the
  [built-in C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
  be enabled.
- `${workspaceFolder}` is the full path to Chromium's `src` directory.
- `your-executable-location` will be one of the following depending on a few items:
  - `Testing`: If you are using the default settings of
    [Electron's Build-Tools](https://github.com/electron/build-tools) or the default
    instructions when [building from source](../development/build-instructions-gn.md#building).
  - `Release`: If you built a Release build rather than a Testing build.
  - `your-directory-name`: If you modified this during your build process from
    the default, this will be whatever you specified.
- The `args` array string `"your-electron-project-path"` should be the absolute
  path to either the directory or `main.js` file of the Electron project you are
  using for testing. In this example, it should be your path to `my-app`.

#### 3. Debugging

Set some breakpoints in the .cc files of your choosing in the native Electron C++
code, and start debugging in the [Debug View](https://code.visualstudio.com/docs/editor/debugging).
