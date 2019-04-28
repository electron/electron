<h1>What is Electron?</h1>
<p>Quoth the about page, "Electron is an open source library developed by GitHub for building cross-platform desktop applications with HTML, CSS, and JavaScript. Electron accomplishes this by combining Chromium and Node.js into a single runtime and apps can be packaged for Mac, Windows, and Linux."</p>

<h3>I didn't understand any of that!</h3>
Let me put it another way. Imagine you are writing a Java program. Let's say you need to use threads. You can just type import java.net.* to get all the libraries necessary to do so. That is because you are working in a <strong>Java Runtime Environment</strong>, or a <strong>Java runtime</strong>. Said runtime provides you with packages to use and import while you work, rather than having to download.
With that in mind, Electron is like that, but instead of Java, it runs a combination of <a href="https://en.wikipedia.org/wiki/Chromium_(web_browser)">Chromium</a> (Google's open source browser), and <a href="https://www.w3schools.com/nodejs/nodejs_intro.asp">Node.js</a> (a runtime for Javascript that interacts well with web browsers). What that means is that Electron uses web-based development to develop powerful desktop applications.

<h3>That sounds cool! How can I try it out?</h3>
<p>First, you want to <a href="https://electronjs.org/docs/tutorial/installation">install Electron.</a>.
Once it's installed, here's a <a href="https://electronjs.org/docs/tutorial/first-app">guide to writing your first app</a>. For subsequent apps, since it uses Javascript and runs similarly to Node.js, here is a tutorial for <a href="https://www.w3schools.com/js/">Javascript</a> and for <a href="https://www.w3schools.com/nodejs/"> Node.js</a>.</p>
<p>Application not working? Here are the guides on <a href="https://electronjs.org/docs/tutorial/application-debugging">general debugging</a>, <a href="https://electronjs.org/docs/tutorial/debugging-main-process">debugging the main process</a>, and <a href="https://electronjs.org/docs/tutorial/debugging-main-process-vscode">debugging the main process in Visual Studio Code</a>.</p>

<h3>I got my first app working. Now what?</h3>
What's next is to get familiar with how Electron works and how to write apps in it. To that end, the first thing to do is study <a href="https://electronjs.org/docs/tutorial/application-architecture">its architecture</a>. That page also lists its APIs, which can be found <a href="https://electronjs.org/docs">here.</a>
You will also need to set up a <a href="https://electronjs.org/docs/tutorial/development-environment">developer environment</a>, and while less necessary, <a href="https://electronjs.org/docs/tutorial/devtools-extension">DevTools</a> can be helpful when programming.
In addition, if you want to make desktop apps, you may need to set up a <a href="https://electronjs.org/docs/tutorial/desktop-environment-integration>desktop environment</a>.
Now that we can start programming our own apps, we need to start somewhere-which can be helped with a <a href="https://electronjs.org/docs/tutorial/boilerplates-and-clis">boilerplate</a>. 
Now, while we're programming, it's always good to know the <a href="https://electronjs.org/docs/tutorial/keyboard-shortcuts">keyboard shortcuts</a>.
Finally, there's <a href="https://electronjs.org/docs/tutorial/testing-on-headless-ci">testing the app</a>.

<h3>I have a pretty good foundation to build apps on. What next?</h3>
Well, there are a lot of tutorials <a href="https://electronjs.org/docs">here</a>, including things like making accessible applications, or modifying the taskbar at the bottom. There are too many to go through one by one, but I suggest starting with application distribution and code signing.

<h3>Miscellaneous</h3>
If you want to support Electron, the project welcomes <a href="https://electronjs.org/docs/tutorial/app-feedback-program">feedback</a> and <a href="https://github.com/electron/electron/blob/master/CONTRIBUTING.md"> OSS contributions to issues.</a>
