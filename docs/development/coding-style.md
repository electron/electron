# Coding Style

These are the style guidelines for coding in Electron.

## C++ and Python

For C++ and Python, we follow Chromium's [Coding
Style](http://www.chromium.org/developers/coding-style). There is also a
script `script/cpplint.py` to check whether all files conform.

The Python version we are using now is Python 2.7.

The C++ code uses a lot of Chromium's abstractions and types, so it's
recommended to get acquainted with them. A good place to start is
Chromium's [Important Abstractions and Data Structures](https://www.chromium.org/developers/coding-style/important-abstractions-and-data-structures)
document. The document mentions some special types, scoped types (that
automatically release their memory when going out of scope), logging mechanisms
etc.

## CoffeeScript

For CoffeeScript, we follow GitHub's [Style
Guide](https://github.com/styleguide/javascript) and the following rules:

* Files should **NOT** end with new line, because we want to match Google's
  styles.
* File names should be concatenated with `-` instead of `_`, e.g.
  `file-name.coffee` rather than `file_name.coffee`, because in
  [github/atom](https://github.com/github/atom) module names are usually in
  the `module-name` form. This rule only applies to `.coffee` files.

## API Names

When creating a new API, we should prefer getters and setters instead of
jQuery's one-function style. For example, `.getText()` and `.setText(text)`
are preferred to `.text([text])`. There is a
[discussion](https://github.com/atom/electron/issues/46) on this.
