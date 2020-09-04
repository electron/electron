# Debugging Native C++ in VSCode on Windows

### 1. Open an Electron project in VSCode.

```sh
$ git clone git@github.com:electron/electron-quick-start.git
$ code electron-quick-start
```

### 2. Add a file `.vscode/launch.json` with the following configuration:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "(Windows) Launch",
      "type": "cppvsdbg",
      "request": "launch",
      "program": "${workspaceFolder}/out/Testing/electron.exe",
      "args": [],
      "stopAtEntry": false,
      "cwd": "${workspaceFolder}",
      "environment": [
          {"name": "ELECTRON_ENABLE_LOGGING", "value": "true"},
          {"name": "ELECTRON_ENABLE_STACK_DUMPING", "value": "true"},
          {"name": "ELECTRON_RUN_AS_NODE", "value": ""},
      ],
      "externalConsole": false,
      "sourceFileMap": {
          "o:\\": "${workspaceFolder}",
      },
    },
  ]
}
```
"args" should also have one parameter, a string of the path to your your Electron project's folder. In this example, it should be your path to electron-quick-start.

### 3. Debugging

Set some breakpoints in the .cc files of your choosing in the native Electron C++ code, and start debugging in the [Debug View](https://code.visualstudio.com/docs/editor/debugging).