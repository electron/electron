# Supported Chrome command line switches

Following command lines switches in Chrome browser are also Supported in
Electron, you can use [app.commandLine.appendSwitch][append-switch] to append
them in your app's main script before the [ready][ready] event of [app][app]
module is emitted:

```javascript
var app = require('app');
app.commandLine.appendSwitch('remote-debugging-port', '8315');
app.commandLine.appendSwitch('host-rules', 'MAP * 127.0.0.1');

app.on('ready', function() {
});
```

## --disable-http-cache

Disables the disk cache for HTTP requests.

## --remote-debugging-port=`port`

Enables remote debug over HTTP on the specified `port`.

## --proxy-server=`address:port`

Uses a specified proxy server, overrides system settings. This switch only
affects HTTP and HTTPS requests.

## --no-proxy-server

Don't use a proxy server, always make direct connections. Overrides any other
proxy server flags that are passed.

## --host-rules=`rules`

Comma-separated list of `rules` that control how hostnames are mapped.

For example:

* `MAP * 127.0.0.1` Forces all hostnames to be mapped to 127.0.0.1
* `MAP *.google.com proxy` Forces all google.com subdomains to be resolved to
  "proxy".
* `MAP test.com [::1]:77` Forces "test.com" to resolve to IPv6 loopback. Will
  also force the port of the resulting socket address to be 77.
* `MAP * baz, EXCLUDE www.google.com` Remaps everything to "baz", except for
  "www.google.com".

These mappings apply to the endpoint host in a net request (the TCP connect
and host resolver in a direct connection, and the `CONNECT` in an http proxy
connection, and the endpoint host in a `SOCKS` proxy connection).

## --host-resolver-rules=`rules`

Like `--host-rules` but these `rules` only apply to the host resolver.

[app]: app.md
[append-switch]: app.md#appcommandlineappendswitchswitch-value
[ready]: app.md#event-ready

## --ignore-certificate-errors

Ignore certificate related errors.

## --ppapi-flash-path

Set path to pepper flash plugin for use.

## --ppapi-flash-version

Set the pepper flash version.

## --v=`log_level`

Gives the default maximal active V-logging level; 0 is the default. Normally
positive values are used for V-logging levels.

Passing `--v=-1` will disable logging.

## --vmodule=`pattern`

Gives the per-module maximal V-logging levels to override the value given by
`--v`. E.g. `my_module=2,foo*=3` would change the logging level for all code in
source files `my_module.*` and `foo*.*`.

Any pattern containing a forward or backward slash will be tested against the
whole pathname and not just the module. E.g. `*/foo/bar/*=2` would change the
logging level for all code in source files under a `foo/bar` directory.

To disable all chromium related logs and only enable your application logs you
can do:

```javascript
app.commandLine.appendSwitch('v', -1);
app.commandLine.appendSwitch('vmodule', 'console=0');
```
