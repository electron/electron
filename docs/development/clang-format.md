# Using clang-format on Electron C++ Code

[`clang-format`](http://clang.llvm.org/docs/ClangFormat.html) is a tool to
automatically format C/C++/Objective-C code, so that developers don't need to
concern about style issues during code review.

It is highly recommended to format your changed C++ code before sending to
code review, which will save your and reviews' time.

To automatically format a file according to Electron C++ code style, simply run
`clang-format path/to/electron/file.cc`. It should work on macOS/Linux/Windows.

The workflow to format your changed code:

1. Do your change in electron repository.
2. Run `git add your_changed_file.cc`.
3. Run `git-clang-format`, and you will probably see modifications in
your_changed_file.cc, these modifications are generated from `clang-format`.
4. Run `git add your_changed_file.cc`, and commit your change.
5. Now it is ready to send a pull request.

If you want to format the changed code on your latest git commit (HEAD), you can
run `git-clang-format HEAD~1`. See `git-clang-format -h` for more details.

You can install `clang-format` and `git-clang-format` via
`npm install -g clang-format`.

## Editor integration

It is useful to integrate `clang-format` tool in your favorite editors.
For further guidance on setting up editor integration, see specific pages:

  * [Atom](https://atom.io/packages/clang-format)
  * [Vim & Emacs](http://clang.llvm.org/docs/ClangFormat.html#vim-integration)
