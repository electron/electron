# web-frame

The `web-frame` module can custom the rendering of current web page.

An example of zooming current page to 200%.

```javascript
var webFrame = require('web-frame');
webFrame.setZoomFactor(2);
```

## webFrame.setZoomFactor(factor)

* `factor` Number - Zoom factor

Changes the zoom factor to the specified factor, zoom factor is
zoom percent / 100, so 300% = 3.0.

## webFrame.getZoomFactor()

Returns the current zoom factor.

## webFrame.setZoomLevel(level)

* `level` Number - Zoom level

Changes the zoom level to the specified level, 0 is "original size", and each
increment above or below represents zooming 20% larger or smaller to default
limits of 300% and 50% of original size, respectively.

## webFrame.getZoomLevel()

Returns the current zoom level.

## webFrame.setSpellCheckProvider(language, autoCorrectWord, provider)

* `language` String
* `autoCorrectWord` Boolean
* `provider` Object

Sets a provider for spell checking in input fields and text areas.

The `provider` must be an object that has a `spellCheck` method that returns
whether the word passed is correctly spelled.

An example of using [node-spellchecker][spellchecker] as provider:

```javascript
require('web-frame').setSpellCheckProvider("en-US", true, {
  spellCheck: function(text) {
    return !(require('spellchecker').isMisspelled(text));
  }
});
```

## webFrame.registerUrlSchemeAsSecure(scheme)

* `scheme` String

Registers the `scheme` as secure scheme.

Secure schemes do not trigger mixed content warnings. For example, `https` and
`data` are secure schemes because they cannot be corrupted by active network
attackers.

## webFrame.registerUrlSchemeAsBypassingCsp(scheme)

* `scheme` String

Resources will be loaded from this `scheme` regardless of
page's Content Security Policy.

[spellchecker]: https://github.com/atom/node-spellchecker
