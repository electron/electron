# 支持的 Chrome 命令行开关

这页列出了Chrome浏览器和Electron支持的命令行开关. 你也可以在[app][app]模块的[ready][ready]事件发出之前使用[app.commandLine.appendSwitch][append-switch] 来添加它们到你应用的main脚本里面:

```javascript
const app = require('electron').app;
app.commandLine.appendSwitch('remote-debugging-port', '8315');
app.commandLine.appendSwitch('host-rules', 'MAP * 127.0.0.1');

app.on('ready', function() {
  // Your code here
});
```

## --client-certificate=`path`

设置客户端的证书文件 `path` .

## --ignore-connections-limit=`domains`

忽略用 `,` 分隔的 `domains` 列表的连接限制.

## --disable-http-cache

禁止请求 HTTP 时使用磁盘缓存.

## --remote-debugging-port=`port`

在指定的 `端口` 通过 HTTP 开启远程调试.

## --js-flags=`flags`

指定引擎过渡到 JS 引擎. 

在启动Electron时，如果你想在主进程中激活 `flags` ，它将被转换.

```bash
$ electron --js-flags="--harmony_proxies --harmony_collections" your-app
```

## --proxy-server=`address:port`

使用一个特定的代理服务器，它将比系统设置的优先级更高.这个开关只有在使用 HTTP 协议时有效，它包含 HTTPS 和 WebSocket 请求. 值得注意的是，不是所有的代理服务器都支持 HTTPS 和 WebSocket 请求.

## --proxy-bypass-list=`hosts`

让 Electron 使用(原文:bypass) 提供的以 semi-colon 分隔的hosts列表的代理服务器.这个开关只有在使用 `--proxy-server` 时有效.

例如:

```javascript
app.commandLine.appendSwitch('proxy-bypass-list', '<local>;*.google.com;*foo.com;1.2.3.4:5678')
```


将会为所有的hosts使用代理服务器，除了本地地址 (`localhost`,
`127.0.0.1` etc.), `google.com` 子域, 以 `foo.com` 结尾的hosts，和所有类似 `1.2.3.4:5678`的.

## --proxy-pac-url=`url`

在指定的 `url` 上使用 PAC 脚本.

## --no-proxy-server

不使用代理服务并且总是使用直接连接.忽略所有的合理代理标志.

## --host-rules=`rules`

一个逗号分隔的 `rule` 列表来控制主机名如何映射.

例如:

* `MAP * 127.0.0.1` 强制所有主机名映射到 127.0.0.1
* `MAP *.google.com proxy` 强制所有 google.com 子域 使用 "proxy".
* `MAP test.com [::1]:77` 强制 "test.com" 使用 IPv6 回环地址. 也强制使用端口 77.
* `MAP * baz, EXCLUDE www.google.com` 重新全部映射到 "baz", 除了
  "www.google.com".

这些映射适用于终端网络请求
(TCP 连接
和 主机解析 以直接连接的方式, 和 `CONNECT` 以代理连接, 还有 终端 host 使用 `SOCKS` 代理连接).

## --host-resolver-rules=`rules`

类似 `--host-rules` ，但是 `rules` 只适合主机解析.

## --ignore-certificate-errors

忽略与证书相关的错误.

## --ppapi-flash-path=`path`

设置Pepper Flash插件的路径 `path` .

## --ppapi-flash-version=`version`

设置Pepper Flash插件版本号.

## --log-net-log=`path`

使网络日志事件能够被读写到 `path`.

## --ssl-version-fallback-min=`version`

设置最简化的 SSL/TLS 版本号 ("tls1", "tls1.1" or "tls1.2")，TLS 可接受回退.

## --cipher-suite-blacklist=`cipher_suites`

指定逗号分隔的 SSL 密码套件 列表实效.

## --disable-renderer-backgrounding

防止 Chromium 降低隐藏的渲染进程优先级.

这个标志对所有渲染进程全局有效，如果你只想在一个窗口中禁止使用，你可以采用 hack 方法[playing silent audio][play-silent-audio].

## --enable-logging

打印 Chromium 信息输出到控制台.

如果在用户应用加载完成之前解析`app.commandLine.appendSwitch` ，这个开关将实效，但是你可以设置 `ELECTRON_ENABLE_LOGGING` 环境变量来达到相同的效果.

## --v=`log_level`

设置默认最大活跃 V-logging 标准; 默认为 0.通常 V-logging 标准值为肯定值.

这个开关只有在 `--enable-logging` 开启时有效.

## --vmodule=`pattern`

赋予每个模块最大的 V-logging levels 来覆盖 `--v` 给的值.E.g. `my_module=2,foo*=3` 会改变所有源文件 `my_module.*` and `foo*.*` 的代码中的 logging level .

任何包含向前的(forward slash)或者向后的(backward slash)模式将被测试用于阻止整个路径名，并且不仅是E.g模块.`*/foo/bar/*=2` 将会改变所有在 `foo/bar` 下的源文件代码中的 logging level .

这个开关只有在 `--enable-logging` 开启时有效.

[app]: app.md
[append-switch]: app.md#appcommandlineappendswitchswitch-value
[ready]: app.md#event-ready
[play-silent-audio]: https://github.com/atom/atom/pull/9485/files
