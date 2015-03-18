# Technical differences to NW.js (formerly node-webkit)

Like NW.js, atom-shell provides a platform to write desktop applications
with JavaScript and HTML, and has Node integration to grant access to low level
system in web pages.

But there are also fundamental differences between the two projects that make
atom-shell a completely separate product from NW.js:

**1. Entry of application**

In NW.js, the main entry of an application is a web page, you specify a
main page in the `package.json` and it would be opened in a browser window as
the application's main window.

While in atom-shell, the entry point is a JavaScript script, instead of
providing a URL directly, you need to manually create a browser window and load
html file in it with corresponding API. You also need to listen to window events
to decide when to quit the application.

So atom-shell works more like the Node.js runtime, and APIs are more low level,
you can also use atom-shell for web testing purpose like
[phantomjs](http://phantomjs.org/).

**2. Build system**

In order to avoid the complexity of building the whole Chromium, atom-shell uses
[libchromiumcontent](https://github.com/brightray/libchromiumcontent) to access
Chromium's Content API, libchromiumcontent is a single, shared library that
includes the Chromium Content module and all its dependencies. So users don't
need a powerful machine to build atom-shell.

**3. Node integration**

In NW.js, the Node integration in web pages requires patching Chromium to
work, while in atom-shell we chose a different way to integrate libuv loop to
each platform's message loop to avoid hacking Chromium, see the
[`node_bindings`](../../atom/common/) code for how that was done.

**4. Multi-context**

If you are an experienced NW.js user, you should be familiar with the
concept of Node context and web context, these concepts were invented because
of how the NW.js was implemented.

By using the [multi-context](http://strongloop.com/strongblog/whats-new-node-js-v0-12-multiple-context-execution/)
feature of Node, atom-shell doesn't introduce a new JavaScript context in web
pages.
