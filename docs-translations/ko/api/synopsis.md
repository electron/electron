# 개요

Electron은 모든 [Node.js의 built-in 모듈](http://nodejs.org/api/)과 third-party node 모듈을 완벽하게 지원합니다. ([네이티브 모듈](../tutorial/using-native-node-modules.md) 포함)

Electron은 네이티브 데스크톱 어플리케이션을 개발 할 수 있도록 추가적인 built-in 모듈을 제공합니다.
몇몇 모듈은 메인 프로세스에서만 사용할 수 있고 어떤 모듈은 랜더러 프로세스(웹 페이지)에서만 사용할 수 있습니다.
또한 두 프로세스 모두 사용할 수 있는 모듈도 있습니다.

기본적인 규칙으로 [GUI](https://en.wikipedia.org/wiki/Graphical_user_interface)와 저 수준 시스템에 관련된 모듈들은 오직 메인 프로세스에서만 사용할 수 있습니다.
[메인 프로세스 vs. 랜더러 프로세스](../tutorial/quick-start.md#메인 프로세스) 컨셉에 익숙해야 이 모듈들을 사용하기 쉬우므로 관련 문서를 읽어 보는 것을 권장합니다.

메인 프로세스 스크립트는 일반 Node.js 스크립트와 비슷합니다:

```javascript
var app = require('app');
var BrowserWindow = require('browser-window');

var window = null;

app.on('ready', function() {
  window = new BrowserWindow({width: 800, height: 600});
  window.loadUrl('https://github.com');
});
```

랜더러 프로세스도 예외적인 node module들을 사용할 수 있다는 점을 제외하면 일반 웹 페이지와 크게 다를게 없습니다:

```html
<!DOCTYPE html>
<html>
  <body>
    <script>
      var remote = require('remote');
      console.log(remote.require('app').getVersion());
    </script>
  </body>
</html>
```

어플리케이션을 실행하려면 [앱 실행하기](../tutorial/quick-start.md#앱 실행하기) 문서를 참고하기 바랍니다.
