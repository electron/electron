# 접근성

접근 가능한 애플리케이션을 만드는 것은 중요합니다. 우리는 새 기능
[Devtron](https://electron.atom.io/devtron) 과
[Spectron](https://electron.atom.io/spectron) 을 소개할 수 있어 기쁩니다.
이것들은 개발자가 모두에게 더 좋은 앱을 만들 수 있는 기회를 제공합니다.

---

Electron 애플리케이션의 접근성에 대한 관심은 두 웹사이트가 유사하며 궁극적으로
HTML 입니다. 그러나 검사를 위한 URL 이 없기 때문에 Electron 앱에서 접근성
검사에 온라인 자원을 사용할 수 없습니다.

이 새 기능들은 Electron 앱에 검사 도구를 제공합니다. Spectron 으로 테스트 하기
위한 검사를 추가 하거나 Devtron 으로 개발자 도구의 것을 사용할 수 있습니다.
자세한 정보는 도구의 요약이나
[접근성 문서](http://electron.atom.io/docs/tutorial/accessibility) 를 읽어보세요.

### Spectron

테스트 프레임워크 Spectron 을 통해 애플리케이션의 각 창과 `<webview>` 태그를
검사할 수 있습니다. 예시입니다:

```javascript
app.client.auditAccessibility().then(function (audit) {
  if (audit.failed) {
    console.error(audit.message)
  }
})
```

이 기능에 대한 자세한 내용은
[Spectron 의 문서](https://github.com/electron/spectron#accessibility-testing)
를 참고하세요.

### Devtron

Devtron 에서 앱의 페이지를 검사할 수 있는 접근성 탭이 생겼으며, 결과를 정렬하고
필터할 수 있습니다.

![devtron screenshot](https://cloud.githubusercontent.com/assets/1305617/17156618/9f9bcd72-533f-11e6-880d-389115f40a2a.png)

이 두 도구들은 구글이 크롬을 위해 개발한
[접근성 개발자 도구](https://github.com/GoogleChrome/accessibility-developer-tools)
라이브러리를 사용합니다. 이 라이브러리에서 사용한 접근성 검사 규칙은
[저장소의 위키](https://github.com/GoogleChrome/accessibility-developer-tools/wiki/Audit-Rules)를
통해 더 알아볼 수 있습니다.

Electron 을 위한 다른 훌륭한 접근성 도구를 알고계시다면,
[접근성 문서](http://electron.atom.io/docs/tutorial/accessibility) 에 풀
요청과 함께 추가해 주세요.
