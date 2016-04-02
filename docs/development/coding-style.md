# Coding Style

These are the style guidelines for coding in Electron.

You can run `npm run lint` to show any style issues detected by `cpplint` and
`eslint`.

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

## JavaScript

* Write [standard](http://npm.im/standard) JavaScript style.
* Files should **NOT** end with new line, because we want to match Google's
  styles.
* File names should be concatenated with `-` instead of `_`, e.g.
  `file-name.js` rather than `file_name.js`, because in
  [github/atom](https://github.com/github/atom) module names are usually in
  the `module-name` form. This rule only applies to `.js` files.
* Use newer ES6/ES2015 syntax where appropriate
  * [`const`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/const)
    for requires and other constants
  * [`let`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/let)
    for defining variables
  * [Arrow functions](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Functions/Arrow_functions)
    instead of `function () { }`
  * [Template literals](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Template_literals)
    instead of string concatenation using `+`

## API Names

When creating a new API, we should prefer getters and setters instead of
jQuery's one-function style. For example, `.getText()` and `.setText(text)`
are preferred to `.text([text])`. There is a
[discussion](https://github.com/atom/electron/issues/46) on this.
