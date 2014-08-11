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

[x-frame-options](https://developer.mozilla.org/en-US/docs/Web/HTTP/X-Frame-Options)
