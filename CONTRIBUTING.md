# Contributing to Electron

:memo: Available Translations: [Korean](https://github.com/electron/electron/tree/master/docs-translations/ko-KR/project/CONTRIBUTING.md) | [Simplified Chinese](https://github.com/electron/electron/tree/master/docs-translations/zh-CN/project/CONTRIBUTING.md)

:+1::tada: First off, thanks for taking the time to contribute! :tada::+1:

This project adheres to the Contributor Covenant [code of conduct](CODE_OF_CONDUCT.md).
By participating, you are expected to uphold this code. Please report unacceptable
behavior to atom@github.com.

The following is a set of guidelines for contributing to Electron.
These are just guidelines, not rules, use your best judgment and feel free to
propose changes to this document in a pull request.

## Submitting Issues

* You can create an issue [here](https://github.com/electron/electron/issues/new),
but before doing that please read the notes below and include as many details as
possible with your report. If you can, please include:
  * The version of Electron you are using
  * The operating system you are using
  * If applicable, what you were doing when the issue arose and what you
  expected to happen
* Other things that will help resolve your issue:
  * Screenshots and animated GIFs
  * Error output that appears in your terminal, dev tools or as an alert
  * Perform a [cursory search](https://github.com/electron/electron/issues?utf8=✓&q=is%3Aissue+)
  to see if a similar issue has already been submitted

## Submitting Pull Requests

* Include screenshots and animated GIFs in your pull request whenever possible.
* Follow the JavaScript, C++, and Python [coding style defined in docs](/docs/development/coding-style.md).
* Write documentation in [Markdown](https://daringfireball.net/projects/markdown).
  See the [Documentation Styleguide](/docs/styleguide.md).
* Use short, present tense commit messages. See [Commit Message Styleguide](#git-commit-messages).

## Styleguides

### General Code

* End files with a newline.
* Place requires in the following order:
  * Built in Node Modules (such as `path`)
  * Built in Electron Modules (such as `ipc`, `app`)
  * Local Modules (using relative paths)
* Place class properties in the following order:
  * Class methods and properties (methods starting with a `@`)
  * Instance methods and properties
* Avoid platform-dependent code:
  * Use `path.join()` to concatenate filenames.
  * Use `os.tmpdir()` rather than `/tmp` when you need to reference the
    temporary directory.
* Using a plain `return` when returning explicitly at the end of a function.
  * Not `return null`, `return undefined`, `null`, or `undefined`

### Git Commit Messages

* Use the present tense ("Add feature" not "Added feature")
* Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
* Limit the first line to 72 characters or less
* Reference issues and pull requests liberally
* When only changing documentation, include `[ci skip]` in the commit description
* Consider starting the commit message with an applicable emoji:
  * :art: `:art:` when improving the format/structure of the code
  * :racehorse: `:racehorse:` when improving performance
  * :non-potable_water: `:non-potable_water:` when plugging memory leaks
  * :memo: `:memo:` when writing docs
  * :penguin: `:penguin:` when fixing something on Linux
  * :apple: `:apple:` when fixing something on macOS
  * :checkered_flag: `:checkered_flag:` when fixing something on Windows
  * :bug: `:bug:` when fixing a bug
  * :fire: `:fire:` when removing code or files
  * :green_heart: `:green_heart:` when fixing the CI build
  * :white_check_mark: `:white_check_mark:` when adding tests
  * :lock: `:lock:` when dealing with security
  * :arrow_up: `:arrow_up:` when upgrading dependencies
  * :arrow_down: `:arrow_down:` when downgrading dependencies
  * :shirt: `:shirt:` when removing linter warnings
