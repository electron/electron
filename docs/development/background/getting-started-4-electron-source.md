# Electron Source Code

The [electron blog](https://electronjs.org/blog) is also an extremely helpful resource. In particular, I found these to be helpful.

* [Message Loop Integration](https://electronjs.org/blog/electron-internals-node-integration)
* [Using Node as a library](https://electronjs.org/blog/electron-internals-using-node-as-a-library)
* [Weak References](https://electronjs.org/blog/electron-internals-weak-references)
* [Building Chromium as a Library](https://electronjs.org/blog/electron-internals-building-chromium-as-a-library)

I have found certain Electron API's are almost "must knows" because they are essentially wrappers around chromium content api libraries. I'd recommend reading a lot of documentation on [BrowserWindow](https://electronjs.org/docs/api/browser-window), [WebContents](https://electronjs.org/docs/api/web-contents), and [WebFrame](https://electronjs.org/docs/api/web-frame).

[Source code overview](https://github.com/electron/electron/blob/master/docs/development/source-code-directory-structure.md)