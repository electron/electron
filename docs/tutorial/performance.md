# Performance

Developers frequently ask about strategies to optimize the performance of
Electron applications. Software engineers, consumers, and framework developers
do not always agree on one single definition of what "performance" means. This
document outlines some of the Electron maintainers' favorite ways to reduce the
amount of memory, CPU, and disk resources being used while ensuring that your
app is responsive to user input and completes operations as quickly as
possible. Furthermore, we want all performance strategies to maintain a high
standard for your app's security.

Wisdom and information about how to build performant websites with JavaScript
generally applies to Electron apps, too. To a certain extent, resources
discussing how to build performant Node.js applications apply too, but be
careful to understand that the term "performance" means different things for
a Node.js backend than it does for an application running on a client.

This list is provided for your convenience – and is, much like our
[security checklist][security] – not meant to exhaustive. It is probably possible
to build a slow Electron app that follows all the steps outlined below. Electron
is a powerful development platform that enables you, the developer, to do more
or less whatever you want. All that freedom means that performance is largely
your responsibility.

## Measure, Measure, Measure

The list below contains a number of steps that are fairly straight-forward and
easy to implement. However, building the most performant version of your app
will require you to go beyond a number of steps. Instead, you will have to
closely examine all the code running in your app by carefully profiling and
measuring. Where are the bottlenecks? When the user clicks a button, what
operations take up the brunt of the time? While the app is simply idling, which
objects take up the most memory?

Time and time again, we have seen that the most successful strategy for building
a performant Electron app is to profile the running code, find the most
resource-hungry piece of it, and to optimize it. Repeating this seemingly process
over and over again will dramatically reduce your app's performance. Experience
from working with major apps like Visual Studio Code or Slack has shown that
this practice is by far the most reliable strategy to improve performance.

To learn more about how to profile your app's code, familiarize yourself with
the Chrome Developer Tools. For advanced analysis looking at multiple processes
at once, consider the [Chrome Tracing] tool.

## Checklist

Chances are that your app could be a little leaner, faster, and generally less
resource-hungry if you attempt these steps.

## 1) Carelessly including Node.js modules

Before adding a Node.js module to your application, examine said module. How
many dependencies does that module include? What kind of resources does said
module need to simply be called in a `require()` statement? You might find
that the module with the most downloads on `npm` or the most stars on GitHub
is not in fact the leanest or smallest one available.

### Why?

The reasoning behind this recommendation is best illustrated with a real-world
example. During the early days of Electron, reliable detection of network
connectivity was a problem, resulting many apps to use a module that exposed a
simple `isOnline()` method.

That module detected your network connectivity by attempting to reach out to a
number of well-known endpoints. For the list of those endpoints, it depended on
a different module, which also contained a list of well-known ports. This
dependency itself relied on a module containing information about ports, which
came in the form of a JSON file with more than 100,000 lines of content.
Whenever the module was loaded (usually in a `require('module')` statement),
it would load all its dependencies and eventually read and parse this JSON
file. Parsing many thousands lines of JSON is a very expensive operation. On
a slow machine it can take up whole seconds of time.

In many server contexts, startup time is virtually irrelevant. A Node.js server
that requires information about all ports is likely actually "more performant"
if it loads all required information into memory whenever the server boots at
the benefit of serving requests faster. The module discussed in this example is
not a "bad" module. Electron apps, however, should not be loading, parsing, and
storing in memory information that it does not actually need.

In short, a seemingly excellent module written primarily for Node.js servers
running Linux might be bad news for your app's performance. In this particular
example, the correct solution was to use no module at all, and to instead use
connectivity checks included in later versions of Chromium.

### How?

When considering a module, we recommend that you check 1) the size of dependencies
included; 2) resources required to load (`require()`) it; 3) resources required
to perform the action you're interested in.

Generating a CPU profile and a heap memory profile for loading a module can be done
with a single command on the command line. In the example below, we're looking at
the popular module `request`.

```sh
node --cpu-prof --heap-prof -e "require('request')"
```

Executing this command results in a `.cpuprofile` file and a `.heapprofile`
file in the directory you executed it in. Both files can be analyzed using
the Chrome Developer Tools, using the `Performance` and `Memory` tabs
respectively.

![performance-cpu-prof]

![performance-heap-prof]

In this example, on the authors machine, we saw that loading `request` took
almost half a second, whereas `node-fetch` took dramatically less memory
and less than 50ms.

## 2) Loading and running code too soon

If you have expensive setup operations, consider deferring those. Inspect all
the work being executed right after the application starts. Instead of firing
off all operations right away, consider staggering them in a sequence more
closely aligned with the user's journey.

In traditional Node.js development, we're used to putting all our `require()`
statements at the top. If you're currently writing your Electron application
using the same strategy _and_ are using sizable modules that you do not
immediately need, apply the same strategy and defer loading to a more
opportune time.

### Why?

Loading modules is a surprisingly expensive operation, especially on Windows.
When your app starts, it should not make users wait for operations that are
currently not necessary.

This might seem obvious, but many applications tend to do a large amount of
work immediately after the app has launched - like checking for updates,
downloading content used in a later flow, or performing heavy disk i/o
operations.

Let's consider Visual Studio Code as an example. When you open a file, it will
immediately display the file to you without any code highlighting, prioritizing
your ability to interact with the text. Once it has done that work, it will
move on to code highlighting.

### How?

Let's consider an example and assume that your application is parsing files
in the fictitious `.foo` format. In order to do that, it relies on the
equally fictitious `foo-parser` module. In traditional Node.js development,
you might write code that eagerly loads dependencies:

```js
const fs = require('fs')
const fooParser = require('foo-parser')

class Parser {
  constructor() {
    this.files = fs.readdirSync('.')
  }

  getParsedFiles() {
    return fooParser.parse(this.files)
  }
}

const parser = new Parser()

module.exports = { parser }
```

In the above example, we're doing a lot of work that's being executed as soon
as the file is loaded. Do we need to get parsed files right away? Could we
do this work a little later, when `getParsedFiles()` is actually called?

```js
// "fs" is likely already being loaded, so the `require()` call is cheap
const fs = require('fs')

class Parser {
  async getFiles() {
    // Touch the disk as soon as `getFiles` is called, not sooner.
    // Also, ensure that we're not blocking other operations by using
    // the asynchronous version.
    return this.files = this.files || await fs.readdir('.')
  }

  async getParsedFiles() {
    // Our fictitious foo-parser is a big and expensive module to load, so
    // defer that work until we actually need to parse files.
    // Since `require()` comes with a module cache, the `require()` call
    // will only be expensive once - subsequent calls of `getParsedFiles()`
    // will be faster.
    const fooParser = require('foo-parser')
    const files = await this.getFiles()

    return fooParser.parse(files)
  }
}

// This operation is now a lot cheaper than in our previous example
const parser = new Parser()

module.exports = { parser }
```

In short, allocate resources "just in time" rather than allocating them all
when your app starts.

## 3) Blocking the main process

## 4) Blocking the renderer process

## 5) Unnecessary polyfills

## 6)


[security]: ./security.md
[performance-cpu-prof]: ../images/performance-cpu-prof.png
[performance-heap-prof]: ../images/performance-heap-prof.png
