<div class='ten columns release offset-by-one'>
        
        <h6 class='docs-breadcrumb'>v0.34.0 / Development
           / Atom Shell vs Node Webkit </h6>
        

        <h1 id="technical-differences-between-electron-and-nw-js-formerly-node-webkit">Technical Differences Between Electron and NW.js (formerly node-webkit)</h1>

<p><strong>Note: Electron was previously named Atom Shell.</strong></p>

<p>Like NW.js, Electron provides a platform to write desktop applications
with JavaScript and HTML and has Node integration to grant access to the low
level system from web pages.</p>

<p>But there are also fundamental differences between the two projects that make
Electron a completely separate product from NW.js:</p>

<p><strong>1. Entry of Application</strong></p>

<p>In NW.js the main entry point of an application is a web page. You specify a
main page URL in the <code>package.json</code> and it is opened in a browser window as
the application&#39;s main window.</p>

<p>In Electron, the entry point is a JavaScript script. Instead of
providing a URL directly, you manually create a browser window and load
an HTML file using the API. You also need to listen to window events
to decide when to quit the application.</p>

<p>Electron works more like the Node.js runtime. Electron&#39;s APIs are lower level
so you can use it for browser testing in place of <a href="http://phantomjs.org/">PhantomJS</a>.</p>

<p><strong>2. Build System</strong></p>

<p>In order to avoid the complexity of building all of Chromium, Electron uses <a href="https://github.com/brightray/libchromiumcontent"><code>libchromiumcontent</code></a> to access
Chromium&#39;s Content API. <code>libchromiumcontent</code> is a single shared library that
includes the Chromium Content module and all of its dependencies. Users don&#39;t
need a powerful machine to build Electron.</p>

<p><strong>3. Node Integration</strong></p>

<p>In NW.js, the Node integration in web pages requires patching Chromium to
work, while in Electron we chose a different way to integrate the libuv loop
with each platform&#39;s message loop to avoid hacking Chromium. See the
<a href="../../atom/common/"><code>node_bindings</code></a> code for how that was done.</p>

<p><strong>4. Multi-context</strong></p>

<p>If you are an experienced NW.js user, you should be familiar with the
concept of Node context and web context. These concepts were invented because
of how NW.js was implemented.</p>

<p>By using the <a href="http://strongloop.com/strongblog/whats-new-node-js-v0-12-multiple-context-execution/">multi-context</a>
feature of Node, Electron doesn&#39;t introduce a new JavaScript context in web
pages.</p>


      </div>
