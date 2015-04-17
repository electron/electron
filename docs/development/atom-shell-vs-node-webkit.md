# Technical differences to NW.js (formerly node-webkit)

__Note: Electron was previously named Atom Shell.__

Like NW.js, Electron provides a platform to write desktop applications
with JavaScript and HTML, and has Node integration to grant access to low level
system in web pages.

But there are also fundamental differences between the two projects that make
Electron a completely separate product from NW.js:

__1. Entry of application__

In NW.js, the main entry of an application is a web page, you specify a
main page in the `package.json` and it would be opened in a browser window as
the application's main window.

While in Electron, the entry point is a JavaScript script, instead of
providing a URL directly, you need to manually create a browser window and load
html file in it with corresponding API. You also need to listen to window events
to decide when to quit the application.

So Electron works more like the Node.js runtime, and APIs are more low level,
you can also use Electron for web testing purpose like
[phantomjs](http://phantomjs.org/).

__2. Build system__

In order to avoid the complexity of building the whole Chromium, Electron uses
[libchromiumcontent](https://github.com/brightray/libchromiumcontent) to access
Chromium's Content API, libchromiumcontent is a single, shared library that
includes the Chromium Content module and all its dependencies. So users don't
need a powerful machine to build Electron.

__3. Node integration__

In NW.js, the Node integration in web pages requires patching Chromium to
work, while in Electron we chose a different way to integrate libuv loop to
each platform's message loop to avoid hacking Chromium, see the
[`node_bindings`](../../atom/common/) code for how that was done.

__4. Multi-context__

If you are an experienced NW.js user, you should be familiar with the
concept of Node context and web context, these concepts were invented because
of how the NW.js was implemented.

By using the [multi-context](http://strongloop.com/strongblog/whats-new-node-js-v0-12-multiple-context-execution/)
feature of Node, Electron doesn't introduce a new JavaScript context in web
pages.
