# 在 VSCode 裡為主程序除錯

### 1. 用 VSCode 打開 Electron 專案

```bash
$ git clone git@github.com:electron/electron-quick-start.git
$ code electron-quick-start
```

### 2. 加入 `.vscode/launch.json` 檔案，裏面包含以下設定：

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug Main Process",
      "type": "node",
      "request": "launch",
      "cwd": "${workspaceRoot}",
      "runtimeExecutable": "${workspaceRoot}/node_modules/.bin/electron",
      "program": "${workspaceRoot}/main.js"
    }
  ]
}
```

**注意:** 在 Windows 上, 請把 `runtimeExecutable` 換成 `"${workspaceRoot}/node_modules/.bin/electron.cmd"` 。

### 3. 除錯

在 `main.js` 裡設定一些中斷點, 透過 [Debug View](https://code.visualstudio.com/docs/editor/debugging) 開始除錯，中斷點執行過程中必須可以被運行到。

這邊有一些預先設定好的專案，可以讓你下載並直接在 VSCode 除錯: https://github.com/octref/vscode-electron-debug/tree/master/electron-quick-start
