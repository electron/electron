# Technical Differences Between Electron and NW.js

Like [NW.js][nwjs], Electron provides a platform to write desktop applications with web
technologies. Both platforms enable developers to utilize HTML, JavaScript, and
Node.js. On the surface, they seem very similar.

There are however fundamental differences between the two projects that make
Electron a completely separate product from NW.js.

## 1) Entry of Application

In NW.js, the main entry point of an application can be an HTML web page. In
that case, NW.js will open the given entry point in a browser window.

In Electron, the entry point is always a JavaScript script. Instead of providing a
URL directly, you manually create a browser window and load an HTML file using
the API. You also need to listen to window events to decide when to quit the
application.

Electron works more like the Node.js runtime. Electron's APIs are lower level so
you can use it for browser testing in place of
[PhantomJS](https://phantomjs.org/).

## 2) Node Integration

In NW.js, the Node integration in web pages requires patching Chromium to work,
while in Electron we chose a different way to integrate the `libuv` loop with
each platform's message loop to avoid hacking Chromium. See the
[`node_bindings`][node-bindings] code for how that was done.

## 3) JavaScript Contexts

If you are an experienced NW.js user, you should be familiar with the concept of
Node context and web context. These concepts were invented because of how NW.js
was implemented.

By using the
[multi-context](https://github.com/nodejs/node-v0.x-archive/commit/756b622)
feature of Node, Electron doesn't introduce a new JavaScript context in web
pages.

Note: NW.js has optionally supported multi-context since 0.13.

## 4) Legacy Support

NW.js still offers a "legacy release" that supports Windows XP. It doesn't
receive security updates.

Given that hardware manufacturers, Microsoft, Chromium, and Node.js haven't
released even critical security updates for that system, we have to warn you
that using Windows XP is wildly insecure and outright irresponsible.

However, we understand that requirements outside our wildest imagination may
exist, so if you're looking for something like Electron that runs on Windows XP,
the NW.js legacy release might be the right fit for you.

## 5) Features

There are numerous differences in the amount of supported features. Electron has
a bigger community, more production apps using it, and [a large amount of
userland modules available on npm][electron-modules].

As an example, Electron has built-in support for automatic updates and countless
tools that make the creation of installers easier. As an example in favor of
NW.js, NW.js supports more `Chrome.*` APIs for the development of Chrome Apps.

Naturally, we believe that Electron is the better platform for polished
production applications built with web technologies (like Visual Studio Code,
Slack, or Facebook Messenger); however, we want to be fair to our web technology
friends. If you have feature needs that Electron does not meet, you might want
to try NW.js.

[nwjs]: https://nwjs.io/
[electron-modules]: https://www.npmjs.com/search?q=electron
[node-bindings]: https://github.com/electron/electron/tree/master/lib/common
