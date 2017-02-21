# 安全，本地功能和你的责任

作为 web 开发人员，我们通常喜欢网络安全性更强大的浏览器 - 与我们编写的代码相关的风险相对较小。我们的网站在沙箱中获得有限的权限，我们相信我们的用户可以享受由大量工程师团队构建的浏览器，能够快速响应新发现的安全威胁。

当使用 Electron 时，要知道 Electron 不是一个 Web 浏览器很重要。它允许您使用熟悉的 Web 技术构建功能丰富的桌面应用程序，但是您的代码具有更强大的功能。 JavaScript 可以访问文件系统，用户 shell 等。这允许您构建更高质量的本机应用程序，但是内在的安全风险会随着授予您的代码的额外权力而增加。

考虑到这一点，请注意，在 Electron 不任何处理的情况下显示来自不受信任的来源的任何内容将带来了严重的安全风险。事实上，最流行的 Electron 应用程序（Atom，Slack，Visual Studio Code 等）主要显示本地内容（或没有 Node 集成的可信安全远程内容） - 如果您的应用程序从在线源执行代码，那么您有责任确保代码不是恶意的。

## 报告安全问题

有关如何正确上报 Electron 漏洞的信息，参阅 [SECURITY.md](https://github.com/electron/electron/tree/master/SECURITY.md)

## Chromium 安全问题和升级

尽管 Electron 努力尽快支持新版本的 Chromium，但开发人员应该意识到，升级是一项严肃的工作 - 涉及手动编辑几十个甚至几百个文件。 考虑到当前的资源和贡献，Electron 通常不会是最新版本的 Chromium，总是落后于一两天或几周。

我们认为，我们当前的更新 Chromium 组件的系统在我们可用的资源和构建在框架之上的大多数应用程序的需求之间取得了适当的平衡。 我们绝对有兴趣听听更多关于在 Electron 上构建事物的人的具体用例。 非常欢迎提出请求并且捐助支持我们的努力。

## 除了以上建议

每当您从远程目标收到代码并在本地执行它时，就会存在安全问题。 举个例子，比如在浏览器窗口内显示的远程网站。 如果攻击者以某种方式设法改变所述内容（通过直接攻击源或者通过在应用和实际目的地之间进行攻击），他们将能够在用户的机器上执行本地代码。

> :警告: 在任何情况下都不应该在启用了 Node 集成时加载并执行远程代码. 反而应该只使用本地文件（与应用程序一起打包）来执行 Node 代码。要显示远程内容, 应使用 `webview` 标签并确保禁用了 `nodeIntegration`.

#### 检查列表

这并不是万无一失的，但至少，你应该尝试以下内容：

* 只显示安全的内容(https)
* 在显示远程内容的所有渲染器中禁用 Node 集成 (在 `webPreferences` 中设置 `nodeIntegration` 为 `false`)
* 在显示远程内容的所有渲染器中启用上下文隔离 (在 `webPreferences` 中设置 `contextIsolation` 为 `true`)
* 在所有加载远程内容的会话中使用 `ses.setPermissionRequestHandler()` .
* 不要禁用 `webSecurity`. 禁用它将禁用同源策略.
* 定义一个 [`Content-Security-Policy`](http://www.html5rocks.com/en/tutorials/security/content-security-policy/)
, 并使用限制规则 (即： `script-src 'self'`)
* 覆盖并禁用 [`eval`](https://github.com/nylas/N1/blob/0abc5d5defcdb057120d726b271933425b75b415/static/index.js#L6-L8)
, 它允许字符串作为代码执行.
* 不要设置 `allowRunningInsecureContent` 为 `true`.
* 不要启用 `experimentalFeatures` 或 `experimentalCanvasFeatures` 除非你知道你在做什么.
* 不要使用 `blinkFeatures` 除非你知道你在做什么.
* WebViews: 不要填加 `nodeintegration` 属性.
* WebViews: 不要使用 `disablewebsecurity`
* WebViews: 不要使用 `allowpopups`
* WebViews: 不要使用 `insertCSS` 或 `executeJavaScript` 操作远程 CSS/JS.

强调一下，这份列表只是将风险降到最低，并不会完全屏蔽风险。 如果您的目的是展示一个网站，浏览器将是一个更安全的选择。