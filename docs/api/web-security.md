# Web Security

Because atom-shell has added node integration to normal web pages, there are
some security adjustments that made atom-shell both more safe and more
convenient.

## Overriding `X-Frame-Options` header

May websites (including Google and Youtube) use the
[X-Frame-Options](x-frame-options) header to disable access to their websites
in `iframe`s. In atom-shell you can add a `disable-x-frame-options` string in
the `iframe`'s name to disable this:

```html
<!-- Refused to display -->
<iframe name="google" src="https://google.com"></iframe>
<!-- Loads as expected -->
<iframe name="google-disable-x-frame-options" src="https://google.com"></iframe>
```

## Frames are sandboxed by default

In normal browsers, `iframe`s are not sandboxed by default, which means a remote
page in `iframe` can easily access its parent's JavaScript context.

In atom-shell because the parent frame may have the power to access native
resources, this could cause security problems. In order to fix it, `iframe`s
in atom-shell are sandboxed with all permissions except the `allow-same-origin`
by default.

If you want to enable things like `parent.window.process.exit()` in `iframe`s,
you need to explicitly add `allow-same-origin` to the `sandbox` attribute, or
just set `sandbox` to `none`:

```html
<iframe sandbox="none" src="https://github.com"></iframe>
```

## Node integration in frames

The `node-integration` option of [BrowserWindow](browser-window.md) controls
whether node integration is enabled in web page and its `iframe`s.

By default the `node-integration` option is `except-iframe`, which means node
integration is disabled in all `iframe`s. You can also set it to `all`, with
which node integration is available to the main page and all its `iframe`s, or
`manual-enable-iframe`, which is like `except-iframe`, but enables `iframe`s
whose name contains string `enable-node-integration`. And setting to `disable`
would disable the node integration in both the main page and its `iframe`s.

An example of enable node integration in `iframe` with `node-integration` set to
`manual-enable-iframe`:

```html
<!-- iframe with node integration enabled -->
<iframe name="gh-enable-node-integration" src="https://github.com"></iframe>

<!-- iframe with node integration disabled -->
<iframe src="http://jandan.net"></iframe>
```

[x-frame-options](https://developer.mozilla.org/en-US/docs/Web/HTTP/X-Frame-Options)
