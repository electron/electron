Technical Differences Between Electron and NW.js (formerly node-webkit)

Note: Electron was previously named Atom Shell.

Like NW.js, Electron provides a platform to write desktop applications with JavaScript and HTML and has Node integration to grant access to the low level system from web pages.

But there are also fundamental differences between the two projects that make Electron a completely separate product from NW.js:

1. Entry of Application

In NW.js the main entry point of an application is a web page. You specify a main page URL in the package.json and it is opened in a browser window as the application's main window.

In Electron, the entry point is a JavaScript script. Instead of providing a URL directly, you manually create a browser window and load an HTML file using the API. You also need to listen to window events to decide when to quit the application.

Electron works more like the Node.js runtime. Electron's APIs are lower level so you can use it for browser testing in place of PhantomJS.

2. Build System

In order to avoid the complexity of building all of Chromium, Electron uses libchromiumcontent to access Chromium's Content API. libchromiumcontent is a single shared library that includes the Chromium Content module and all of its dependencies. Users don't need a powerful machine to build Electron.
