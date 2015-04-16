# Coding style

## C++ and Python

For C++ and Python, we just follow Chromium's [Coding
Style](http://www.chromium.org/developers/coding-style), there is also a
script `script/cpplint.py` to check whether all files confirm.

The python's version we are using now is Python 2.7.

## CoffeeScript

For CoffeeScript, we follow GitHub's [Style
Guide](https://github.com/styleguide/javascript), and also following rules:

* Files should **NOT** end with new line, because we want to match Google's
  styles.
* File names should be concatenated with `-` instead of `_`, e.g.
  `file-name.coffee` rather than `file_name.coffee`, because in
  [github/atom](https://github.com/github/atom) module names are usually in
  the `module-name` form, this rule only apply to `.coffee` files.

## API Names

When creating a new API, we should prefer getters and setters instead of
jQuery's one-function style, for example, `.getText()` and `.setText(text)`
are preferred to `.text([text])`. There is a
[discussion](https://github.com/atom/electron/issues/46) of this.
