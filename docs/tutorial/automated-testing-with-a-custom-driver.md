# Automated Testing with a Custom Driver

To write automated tests for your Electron app, you will need a way to "drive" your application. [Spectron](https://electronjs.org/spectron) is a commonly-used solution which lets you emulate user actions via [WebDriver](http://webdriver.io/). However, it's also possible to write your own custom driver using node's builtin IPC-over-STDIO. The benefit of a custom driver is that it tends to require less overhead than Spectron, and lets you expose custom methods to your test suite.

To create a custom driver, we'll use Node.js' [child_process](https://nodejs.org/api/child_process.html) API. The test suite will spawn the Electron process, then establish a simple messaging protocol:

```js
const childProcess = require('child_process')
const electronPath = require('electron')

// spawn the process
let env = { /* ... */ }
let stdio = ['inherit', 'inherit', 'inherit', 'ipc']
let appProcess = childProcess.spawn(electronPath, ['./app'], { stdio, env })

// listen for IPC messages from the app
appProcess.on('message', (msg) => {
  // ...
})

// send an IPC message to the app
appProcess.send({ my: 'message' })
```

From within the Electron app, you can listen for messages and send replies using the Node.js [process](https://nodejs.org/api/process.html) API:

```js
// listen for IPC messages from the test suite
process.on('message', (msg) => {
  // ...
})

// send an IPC message to the test suite
process.send({ my: 'message' })
```

We can now communicate from the test suite to the Electron app using the `appProcess` object.

For convenience, you may want to wrap `appProcess` in a driver object that provides more high-level functions. Here is an example of how you can do this:

```js
class TestDriver {
  constructor ({ path, args, env }) {
    this.rpcCalls = []

    // start child process
    env.APP_TEST_DRIVER = 1 // let the app know it should listen for messages
    this.process = childProcess.spawn(path, args, { stdio: ['inherit', 'inherit', 'inherit', 'ipc'], env })

    // handle rpc responses
    this.process.on('message', (message) => {
      // pop the handler
      let rpcCall = this.rpcCalls[message.msgId]
      if (!rpcCall) return
      this.rpcCalls[message.msgId] = null
      // reject/resolve
      if (message.reject) rpcCall.reject(message.reject)
      else rpcCall.resolve(message.resolve)
    })

    // wait for ready
    this.isReady = this.rpc('isReady').catch((err) => {
      console.error('Application failed to start', err)
      this.stop()
      process.exit(1)
    })
  }

  // simple RPC call
  // to use: driver.rpc('method', 1, 2, 3).then(...)
  async rpc (cmd, ...args) {
    // send rpc request
    let msgId = this.rpcCalls.length
    this.process.send({ msgId, cmd, args })
    return new Promise((resolve, reject) => this.rpcCalls.push({ resolve, reject }))
  }

  stop () {
    this.process.kill()
  }
}
```

In the app, you'd need to write a simple handler for the RPC calls:

```js
if (process.env.APP_TEST_DRIVER) {
  process.on('message', onMessage)
}

async function onMessage ({ msgId, cmd, args }) {
  let method = METHODS[cmd]
  if (!method) method = () => new Error('Invalid method: ' + cmd)
  try {
    let resolve = await method(...args)
    process.send({ msgId, resolve })
  } catch (err) {
    let reject = {
      message: err.message,
      stack: err.stack,
      name: err.name
    }
    process.send({ msgId, reject })
  }
}

const METHODS = {
  isReady () {
    // do any setup needed
    return true
  }
  // define your RPC-able methods here
}
```

Then, in your test suite, you can use your test-driver as follows:

```js
const test = require('ava')
const electronPath = require('electron')

let app = new TestDriver({
  path: electronPath,
  args: ['./app'],
  env: {
    NODE_ENV: 'test'
  }
})
test.before(async t => {
  await app.isReady
})
test.after.always('cleanup', async t => {
  await app.stop()
})
```
