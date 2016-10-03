# VSCode 에서 메인 프로세스 디버깅하기

### 1. VS Code 에서 Electron 프로젝트 열기.

```bash
$ git clone git@github.com:electron/electron-quick-start.git
$ code electron-quick-start
```

### 2. 다음 설정으로 `.vscode/launch.json` 파일 추가하기:

```javascripton
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

**참고:** 윈도우에서, `runtimeExecutable` 을 위해 `"${workspaceRoot}/node_modules/.bin/electron.cmd"` 를 사용하세요.

### 3. 디버깅

`main.js` 에 중단점을 설정하고,
[Debug View](https://code.visualstudio.com/docs/editor/debugging) 에서 디버깅을
시작하세요. 중단점을 만나게 될 것 입니다.

VSCode 에서 바로 디버깅 할 수 있는 프로젝트를 미리 준비했습니다:
https://github.com/octref/vscode-electron-debug/tree/master/electron-quick-start
