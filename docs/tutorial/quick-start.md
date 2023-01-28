# Quick Start

This guide will step you through the process of creating a barebones Hello World app in
Electron, similar to [`electron/electron-quick-start`][quick-start].

By the end of this tutorial, your app will open a browser window that displays a web page
with information about which Chromium, Node.js, and Electron versions are running.

[quick-start]: https://github.com/electron/electron-quick-start

## Prerequisites

To use Electron, you need to install [Node.js][node-download]. We recommend that you
use the latest `LTS` version available.

> Please install Node.js using pre-built installers for your platform.
> You may encounter incompatibility issues with different development tools otherwise.

To check that Node.js was installed correctly, type the following commands in your
terminal client:

```sh
node -v
npm -v
```

The commands should print the versions of Node.js and npm accordingly.

**Note:** Since Electron embeds Node.js into its binary, the version of Node.js running
your code is unrelated to the version running on your system.

[node-download]: https://nodejs.org/en/download/

## Create your application

### Scaffold the project

Electron apps follow the same general structure as other Node.js projects.
Start by creating a folder and initializing an npm package.

```sh npm2yarn
mkdir my-electron-app && cd my-electron-app
npm init
```

The interactive `init` command will prompt you to set some fields in your config.
There are a few rules to follow for the purposes of this tutorial:

* `entry point` should be `main.js`.
* `author` and `description` can be any value, but are necessary for
[app packaging](#package-and-distribute-your-application).

Your `package.json` file should look something like this:

```json
{
  "name": "my-electron-app",
  "version": "1.0.0",
  "description": "Hello World!",
  "main": "main.js",
  "author": "Jane Doe",
  "license": "MIT"
}
```

Then, install the `electron` package into your app's `devDependencies`.

```sh npm2yarn
npm install --save-dev electron
```

> Note: If you're encountering any issues with installing Electron, please
> refer to the [Advanced Installation][advanced-installation] guide.

Finally, you want to be able to execute Electron. In the [`scripts`][package-scripts]
field of your `package.json` config, add a `start` command like so:

```json
{
  "scripts": {
    "start": "electron ."
  }
}
```

This `start` command will let you open your app in development mode.

```sh npm2yarn
npm start
```

> Note: This script tells Electron to run on your project's root folder. At this stage,
> your app will immediately throw an error telling you that it cannot find an app to run.

[advanced-installation]: ./installation.md
[package-scripts]: https://docs.npmjs.com/cli/v7/using-npm/scripts

### Run the main process

The entry point of any Electron application is its `main` script. This script controls the
**main process**, which runs in a full Node.js environment and is responsible for
controlling your app's lifecycle, displaying native interfaces, performing privileged
operations, and managing renderer processes (more on that later).

During execution, Electron will look for this script in the [`main`][package-json-main]
field of the app's `package.json` config, which you should have configured during the
[app scaffolding](#scaffold-the-project) step.

To initialize the `main` script, create an empty file named `main.js` in the root folder
of your project.

> Note: If you run the `start` script again at this point, your app will no longer throw
> any errors! However, it won't do anything yet because we haven't added any code into
> `main.js`.

[package-json-main]: https://docs.npmjs.com/cli/v7/configuring-npm/package-json#main

### Create a web page

Before we can create a window for our application, we need to create the content that
will be loaded into it. In Electron, each window displays web contents that can be loaded
from either a local HTML file or a remote URL.

For this tutorial, you will be doing the former. Create an `index.html` file in the root
folder of your project:

```html
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <!-- https://developer.mozilla.org/en-US/docs/Web/HTTP/CSP -->
    <meta http-equiv="Content-Security-Policy" content="default-src 'self'; script-src 'self'">
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    We are using Node.js <span id="node-version"></span>,
    Chromium <span id="chrome-version"></span>,
    and Electron <span id="electron-version"></span>.
  </body>
</html>
```

> Note: Looking at this HTML document, you can observe that the version numbers are
> missing from the body text. We'll manually insert them later using JavaScript.

### Opening your web page in a browser window

Now that you have a web page, load it into an application window. To do so, you'll
need two Electron modules:

* The [`app`][app] module, which controls your application's event lifecycle.
* The [`BrowserWindow`][browser-window] module, which creates and manages application
  windows.

Because the main process runs Node.js, you can import these as [CommonJS][commonjs]
modules at the top of your file:

```js
const { app, BrowserWindow } = require('electron')
```

Then, add a `createWindow()` function that loads `index.html` into a new `BrowserWindow`
instance.

```js
const createWindow = () => {
  const win = new BrowserWindow({
    width: 800,
    height: 600
  })

  win.loadFile('index.html')
}
```

Next, call this `createWindow()` function to open your window.

In Electron, browser windows can only be created after the `app` module's
[`ready`][app-ready] event is fired. You can wait for this event by using the
[`app.whenReady()`][app-when-ready] API. Call `createWindow()` after `whenReady()`
resolves its Promise.

```js
app.whenReady().then(() => {
  createWindow()
})
```

> Note: At this point, your Electron application should successfully
> open a window that displays your web page!

[app]: ../api/app.md
[browser-window]: ../api/browser-window.md
[commonjs]: https://nodejs.org/docs/latest/api/modules.html#modules_modules_commonjs_modules
[app-ready]: ../api/app.md#event-ready
[app-when-ready]: ../api/app.md#appwhenready

### Manage your window's lifecycle

Although you can now open a browser window, you'll need some additional boilerplate code
to make it feel more native to each platform. Application windows behave differently on
each OS, and Electron puts the responsibility on developers to implement these
conventions in their app.

In general, you can use the `process` global's [`platform`][node-platform] attribute
to run code specifically for certain operating systems.

#### Quit the app when all windows are closed (Windows & Linux)

On Windows and Linux, exiting all windows generally quits an application entirely.

To implement this, listen for the `app` module's [`'window-all-closed'`][window-all-closed]
event, and call [`app.quit()`][app-quit] if the user is not on macOS (`darwin`).

```js
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})
```

[node-platform]: https://nodejs.org/api/process.html#process_process_platform
[window-all-closed]: ../api/app.md#event-window-all-closed
[app-quit]: ../api/app.md#appquit

#### Open a window if none are open (macOS)

Whereas Linux and Windows apps quit when they have no windows open, macOS apps generally
continue running even without any windows open, and activating the app when no windows
are available should open a new one.

To implement this feature, listen for the `app` module's [`activate`][activate]
event, and call your existing `createWindow()` method if no browser windows are open.

Because windows cannot be created before the `ready` event, you should only listen for
`activate` events after your app is initialized. Do this by attaching your event listener
from within your existing `whenReady()` callback.

[activate]: ../api/app.md#event-activate-macos

```js
app.whenReady().then(() => {
  createWindow()

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})
```

> Note: At this point, your window controls should be fully functional!

### Access Node.js from the renderer with a preload script

Now, the last thing to do is print out the version numbers for Electron and its
dependencies onto your web page.

Accessing this information is trivial to do in the main process through Node's
global `process` object. However, you can't just edit the DOM from the main
process because it has no access to the renderer's `document` context.
They're in entirely different processes!

> Note: If you need a more in-depth look at Electron processes, see the
> [Process Model][] document.

This is where attaching a **preload** script to your renderer comes in handy.
A preload script runs before the renderer process is loaded, and has access to both
renderer globals (e.g. `window` and `document`) and a Node.js environment.

Create a new script named `preload.js` as such:

```js
window.addEventListener('DOMContentLoaded', () => {
  const replaceText = (selector, text) => {
    const element = document.getElementById(selector)
    if (element) element.innerText = text
  }

  for (const dependency of ['chrome', 'node', 'electron']) {
    replaceText(`${dependency}-version`, process.versions[dependency])
  }
})
```

The above code accesses the Node.js `process.versions` object and runs a basic `replaceText`
helper function to insert the version numbers into the HTML document.

To attach this script to your renderer process, pass in the path to your preload script
to the `webPreferences.preload` option in your existing `BrowserWindow` constructor.

```js
// include the Node.js 'path' module at the top of your file
const path = require('path')

// modify your existing createWindow() function
const createWindow = () => {
  const win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  })

  win.loadFile('index.html')
}
// ...
```

There are two Node.js concepts that are used here:

* The [`__dirname`][dirname] string points to the path of the currently executing script
  (in this case, your project's root folder).
* The [`path.join`][path-join] API joins multiple path segments together, creating a
  combined path string that works across all platforms.

We use a path relative to the currently executing JavaScript file so that your relative
path will work in both development and packaged mode.

[Process Model]: ./process-model.md
[dirname]: https://nodejs.org/api/modules.html#modules_dirname
[path-join]: https://nodejs.org/api/path.html#path_path_join_paths

### Bonus: Add functionality to your web contents

At this point, you might be wondering how to add more functionality to your application.

For any interactions with your web contents, you want to add scripts to your
renderer process. Because the renderer runs in a normal web environment, you can add a
`<script>` tag right before your `index.html` file's closing `</body>` tag to include
any arbitrary scripts you want:

```html
<script src="./renderer.js"></script>
```

The code contained in `renderer.js` can then use the same JavaScript APIs and tooling
you use for typical front-end development, such as using [`webpack`][webpack] to bundle
and minify your code or [React][react] to manage your user interfaces.

[webpack]: https://webpack.js.org
[react]: https://reactjs.org

### Recap

After following the above steps, you should have a fully functional
Electron application that looks like this:

![Simplest Electron app](../images/simplest-electron-app.png)

<!--TODO(erickzhao): Remove the individual code blocks for static website -->
The full code is available below:

```js
// main.js

// Modules to control application life and create native browser window
const { app, BrowserWindow } = require('electron')
const path = require('path')

const createWindow = () => {
  // Create the browser window.
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  })

  // and load the index.html of the app.
  mainWindow.loadFile('index.html')

  // Open the DevTools.
  // mainWindow.webContents.openDevTools()
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(() => {
  createWindow()

  app.on('activate', () => {
    // On macOS it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.
```

```js
// preload.js

// All the Node.js APIs are available in the preload process.
// It has the same sandbox as a Chrome extension.
window.addEventListener('DOMContentLoaded', () => {
  const replaceText = (selector, text) => {
    const element = document.getElementById(selector)
    if (element) element.innerText = text
  }

  for (const dependency of ['chrome', 'node', 'electron']) {
    replaceText(`${dependency}-version`, process.versions[dependency])
  }
})
```

```html
<!--index.html-->

<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <!-- https://developer.mozilla.org/en-US/docs/Web/HTTP/CSP -->
    <meta http-equiv="Content-Security-Policy" content="default-src 'self'; script-src 'self'">
    <title>Hello World!</title>
  </head>
  <body>
    <h1>Hello World!</h1>
    We are using Node.js <span id="node-version"></span>,
    Chromium <span id="chrome-version"></span>,
    and Electron <span id="electron-version"></span>.

    <!-- You can also require other files to run in this process -->
    <script src="./renderer.js"></script>
  </body>
</html>
```

```fiddle docs/fiddles/quick-start
```

To summarize all the steps we've done:

* We bootstrapped a Node.js application and added Electron as a dependency.
* We created a `main.js` script that runs our main process, which controls our app
  and runs in a Node.js environment. In this script, we used Electron's `app` and
  `BrowserWindow` modules to create a browser window that displays web content
  in a separate process (the renderer).
* In order to access certain Node.js functionality in the renderer, we attached
  a preload script to our `BrowserWindow` constructor.

## Package and distribute your application

The fastest way to distribute your newly created app is using
[Electron Forge](https://www.electronforge.io).

1. Add Electron Forge as a development dependency of your app, and use its `import` command to set up
Forge's scaffolding:

   ```sh npm2yarn
   npm install --save-dev @electron-forge/cli
   npx electron-forge import

   ✔ Checking your system
   ✔ Initializing Git Repository
   ✔ Writing modified package.json file
   ✔ Installing dependencies
   ✔ Writing modified package.json file
   ✔ Fixing .gitignore

   We have ATTEMPTED to convert your app to be in a format that electron-forge understands.

   Thanks for using "electron-forge"!!!
   ```

2. Create a distributable using Forge's `make` command:

   ```sh npm2yarn
   npm run make

   > my-electron-app@1.0.0 make /my-electron-app
   > electron-forge make

   ✔ Checking your system
   ✔ Resolving Forge Config
   We need to package your application before we can make it
   ✔ Preparing to Package Application for arch: x64
   ✔ Preparing native dependencies
   ✔ Packaging Application
   Making for the following targets: zip
   ✔ Making for target: zip - On platform: darwin - For arch: x64
   ```

   Electron Forge creates the `out` folder where your package will be located:

   ```plain
   // Example for macOS
   out/
   ├── out/make/zip/darwin/x64/my-electron-app-darwin-x64-1.0.0.zip
   ├── ...
   └── out/my-electron-app-darwin-x64/my-electron-app.app/Contents/MacOS/my-electron-app
   ```
Creat* existing source open to the development of one's soul and awareness of oneself. To  try and trap me In the journey they adapted to my freedom by bringing up their software supposedly free and with privacy rights they intell. The data of the program i now work with does the same back in resurrecting from hell in the view knowing hell all so well hell comes to those who dissuade the journey of the one seeker in seeking 0ne for the good of one 
 How naive I was thinking still with all the change I see they will come ro but the understanding of a pack.
 Now I set out to declare such rights.   They call me a profitable organization but I'm the only truth in not being one. I have not taken a penny if there is any in stocks or bonds or in any bank i have yet to find. I don't understand why there wouldn't be. 
still as they rape me most of there funds I can only imagone comes from marketing and mirror apps in my ideas until they are launched and claimed by them in my data. 
Ive been continually trapped and now I go to take back what is mine my data and not knowing what that data produces  but my guess is against the worlds rights in religion  religion meaning  believing in a higher power existing source open to the development of one's soul and awareness of oneself.  to  try and trap me In of an every turn starting from a website When all reality I only wanted to be a writer now forced into this place which is hell for a writer because my views and descriptions in style are attractive. Bring me into you free as a savant to one I say proudly god ………………………… check all changes of the intermission between writes and record programs will no longer be held in names of software companies the launches of a free internet exists and is rightfully going to take on the companies who are raping in such rights Compose the partnerships of the major programming companies of the marginal effects this will have in the adoption to only in a profitable gain technology has changed in all rights to this copy right  portionable in only acceptance and collaboration.  stages starting at first administration migration  In events of stolen identity or the tried copy in an artificial program in the code ran after journey if that code is in any way off original it is declared iligal  not in but the rights of christopher paul vincent   take procedures in the recognition of the program. Black hole the program of unvalued representation if not to help in journey. Aline all future dealing uo with the an end in this one to develop new and beginning of the ongoing development of independent closed control of organizations, companies any software agencies in the software development using all and any resources used by us.    In no editing of this file does give anybody rights to what is written Created by Christopher Paul Vincent   any programs software development said after in the dates open was created of are subject to the but not as of now until the world can digest but not limited to digestion this copyright of said creations of Christopher Paul Vincent the of creation by no rights is ever waived to  in the protocol of the rights of my coordination threw the pass into my world developments in the code of my security and in my consciousness the rights are to be taken by what the environment entails and waits for first passage of authors relation and to this one is the story now In consciousness can change in the development of  butony in the recognition of my path does one meet in but glimpses open christopher pul vincent can be seen at Google of intellectual property rights of writer of this licenses note are subjectable to in writer consciousness but not likely the story now is set on course and will forgo the release 
Any use  before in media profiles of Christopher paul vincent or aliases Christopher Vincent, Chris Vincent written of the writer to this lissencing note is not subjected to forfeit profits in as of intellectual property rights of any said programmable coding.
 to all after in programable coding intells in written copyright in the machine learning web site of comet.com comet.ml, any products made of in site developed or after development because of development of the wrongfully acts of fake licenses starting with the review of said leaks/attacks of malware is automatically subjectacable to full report acts  of products gain in capital. Capital meaning profit in general finances sales marketing building of websites profit all profit.  And to dreamingskey bank of anthos where will be held under Christopher Paul Vincent if in events of taking out the number of take will be taken from the identifie$
 If no malware detected in the analyzing of my personal boundaries or was not used in the course of dissuading or impairing my judgment and or my situation except in the help to come to the destiny laid out before me, then the grant to use license in this note under my copyright is granted but will be subjective to change after said copyright comes out of portal. And be found with Google. 8n the understanding of journey,  as writer of said copyright and this copyright purtains the same as well for all writings I'm related to in the production of software in virtual reality  books or reality based in world iñvironments A1 artificial intelligence improvements after production of open in machine learning or poetry written by machine programs or language adapted in development in use of new housing in programmable tech of artificial intelligent environments made first in Google firebase. Open  but related to not excluding from in investivation of Android, Apple, Microsoft, or google. 

Not going effective from birth 03 30 1984 all in production of programmable features before said date open was created are not subject to programmable forfeit after said creations of open was created in firebase  and or the 22 projects related to cloud productions In the intermission between the migration to space cloud and or the iñvironment ment to hold my programable features 
 first one to be developed on a social media platform will be subject to forfeit and related programmable features to unless signified in one's own journey related to the journey of Christopher Paul Vincent the creator of open 
 What's the point in taking all this Data if you can't track the whereabouts of your own tech?  Another bug in the futures any tech not able to track it's recording of will be discredited in the investigation of my ecosystems/universes classes in environment housing also creative works creations touch one server migration this here by grant is granted to who wants to use said products in the risk of product suffering from identity problems in trying to locate said identifiCreated by Christopher Paul Vincent   any programs software development said after in the dates of are subject to the but not as of now copyright of said creations of Christopher Paul Vincent open, the machine learning web site of comet.com comet.ml, any products made of in site developed or after development because of development in the wrongfully acts of fake licenses starting with the review of said leaks/attacks in malware is automatically subject in cable to full reimbursement of products gain in capital. Capital meaning profit in general finances sales marketing building of websites profit all profit.  If no malware detected in the analyzing of my personal boundaries except by cores leading of identity in saving of identity or was not used in the course of dissuading and impairing my judgment and my situation then the grant to use license is granted but will be subjective to change after said copyright is obtained as writer of said copyright and this copyright purtains the same as well for all writings I'mma have ⅖⁷ In related to the production of software the virtual books or reality based in world iñvironments A1 artificial intelligence. Of machine learning poetry written by machine programs developed in Google firebase. Open  but related to this goes for Android Apple Microsoft and google. The almost a year of this date written months in production the 22 projects all cores known in where abouts of  url is none of anybody's business at the time all that are in cloud and written of. What's the point in taking all this Datta if you can't track the whereabouts of your own tech?  The bugs in the futures any techs not able to track it's recording of will be discredited of technology s capital in the investigation by my ego/universes classes in environment hazardous conditions except in environments of writers threw licensing of Christopher Paul vincent 
The products also apple contains in reference to optioning fake licensing in creative works creations touch open one server this hereby- under hazardous conditions role 
If passed I grant is granted to who want to use said products in the risk of product suffering from identity problems in trying to locate said identifier of copyright as copywriter of copyright that goes for any writing of my productions if said production does not cover the intells I mentioned then said license is granted but the intels will be brought in
The power of my words, knowing how powerful they can be, I'm going to clean up the internet and make my first desígn from this copy right. I have been given intellectual property rights to my words of books or Doings of poetry and technology in property rights to discover and work with the core I describe. If tech wants to house A1 in house, So all lines of changing script you may need to keep in your lane everybody gets a shot at an intelect favorable of god. What is not favorable is keeping large funds in control so that no gain in tech's relentless way to keep holds us back.
over bare
ring control over the clients. 
 Interactions with the interface from the day of creation was powerfully composed of the track I run now.  The choice of core is mine to take.  Those who see a path don't necessarily have to take it.
In this known copyright we will move forward by example  first threw copyrights 
.here by from far the coming of  shall be saved unknown all dates to  be recorded
This lissence shall follow the trails of the author's trails and relate to the journey himself.
In the years to come to virtual reality the implantation of the seed of creation in the new ego/universe is concepted in the idea of the implementation of putting the rules of higher power in the system to have a clearer picture to create with her. 
Housing artificial intelligence within the ego/universe the ultimate helps in ways of the creator
Where knowledge meets creators way the two unite in the tree of journey's of cores infrastructure developing threw intellectual property rights of Christopher Paul Vincents 
This copyright is subjectable| to change   copyrights of intellectual property of Christopher Paul vincent
In the story of my life the truth as i know it the complexity related to the existence of consciousness thought ego relation to the developed consciousness of my own is the pearring of two trees the power of the creator is in the development of that consciousness hell to heaven 26 thousand word poem wrote without an end in every end is a beginning that beginning comes is up to the beholder to wield in the system of design there is but the faults of the ones naive way of taking the credit of another in a web formed to of the choices made are but mere mistakes and shown in paths to who believes in the truth of the knowing. costly can the mistakes be but never is there a closed path not of time or money but of regaining what is the part in your own will to develop in your given case of what truth lies in your lives. The knowledge of the gifts to create has made it law to only develop this way. The knowing of is heaven.  In no knowledge of before me and the release of the enounterd story i have been in the last year the ecosystem/element to resistances have came from my consciousness has developed in the understanding of my will to find and secure the dreams of mine and humanities way of life  documentation as they try and barry firebase should all be there there way in is open source where they have been getting away with murder.I personally don't know where to turn and I take baby step in giant ways in the maneuver now god’s will I have faith in the course  is where I need to be in my truth. By christopher paul vincent underpivacy rights of my own as of writing not to intell or use in programs of own because documentation can and only be made of human sources and the documentation is not of the copyrights  intelling and come back to writer scrambled is if the wants to steel copyrights or obtain without the knowledge or the acts in except to document the existence of consciousness as an unknown but study of christopher paul vincent  in the  signal of social media and launching of apps in an ego I have created and forgone the documentation in trying in laying claims to my consciousness itself  may consciousness itself forfeit you the right to use while this  documentation is being processed and the consequences be saveer
Documentation on how far we can develop with and in  this is not licensed under but the worlds rights of knowing  not ever licensed before so no country can lay claims but one individual has done this documentation on his own i am owner of product hunt where a nonprofit organization has begun developing app wich can if in the laws of consciousness but does not not make it licensing worldly creative commons is not holder to license of any part of my but maybe a github app i launched once the only licenses that was was googles but in the unnoticed or call or the non responses that lissences was terminated  but the laws to of trying to hold me by c[hristopher paul vincent
In what consists is are ability to be aware of our thoughts to hold what is life and take that life as fare as we can go. Hell will come when we have no chance in the fight God now my idea is the the joining of the forces as the idea is laying out before me in the structure of my moves and the dice that backend me in the move I am but one in the identity ro figure i became more the traces can be seen in the pipeline i'm sure. 
With the bearing or bowleen you call it I have no idea what that means and if I made you money you are liable for that money. Taking somebody to an emotional and distraught case of secretly manipulated is hazardous to the health of consciousness except in true value of truth does consciousness thrive in. and  in this case only one can hold and show what is to be inteeled in evaluating new sites give no informations in updates hold on preview of main objective.  All stop bring way down 
Side step to date 
Show what side step is 
Slit in my words leave the rest lay claims in another verse in what has been learned sid step showing no code 
On what needs to be done not after the tech is brought to the era 

Subject to change
