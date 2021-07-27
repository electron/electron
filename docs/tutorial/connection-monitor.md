# Connection Monitor

Many applications require some sort of online/offline detection mechanism.
As detailed in the [Online/Offline Event Detection](./online-offline-events.md)
guide, the Chromium browser does provide a mechanism for determining
connectivity; however, this mechanism is trivial. It only checks for the
presance of a _network_ connection on the machine and not if it can actually
connect to a given resource. This example builds a connection monitoring
service that pings a given resource over a given interval. Depending on the
result of that request, the service changes its state from **CONNECTED** to
**DISCONNECTED** and shares this information with the rest of application.
Additionally, the example includes an secondary guide for adding an
authentication flow using [@azure/msal-node](https://www.npmjs.com/package/@azure/msal-node).

This example was built with the intent to be a starting point for other
projects. You'll most likely have to change things to suit your own needs.
Certain parts of the example were designed for this exact reason. For example,
the `pingOperationService` and the additional `authenticationOperationService`
functions are generalized in a way where you may only need to modify the
internal logic of the function and the state machine can remain the same (more
details later).

The example is built on the [XState](https://github.com/statelyai/xstate)
library. The connection monitor service is a state machine and utilizes the
XState API heavily. Knowledge of state machines is not required to follow along
with this example, but is encouraged. For more information as well as an useful
introduction to state machines, read through the
[XState documentation](https://xstate.js.org/docs/).

## Example

```javascript fiddle='docs/fiddles/connection-monitor/base/main.js'
const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const { interpret } = require('xstate');
const { createConnectionMonitorMachine } = require('./connectionMonitor.js');

const {
  CM_IPC_CHANNEL,
  PING_SERVICE_ID,
  CM_ACTION_TYPES
} = require('./constants.js');

function createWindow () {
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  });

  mainWindow.loadFile('index.html');
  mainWindow.webContents.openDevTools();

  // Create the Connection Monitor machine
  const connectionMonitorMachine = createConnectionMonitorMachine({
    url: 'https://login.microsoftonline.com',
    window: mainWindow
  });

  // Use the XState `interpret` method to create a service out of the machine
  const connectionMonitorService = interpret(connectionMonitorMachine);

  /**
   * Initialize the onTransition listener for the service to forward the
   * machine's state and context to the renderer process. Uses the
   * action/reducer pattern described in other parts of the code and
   * documentation.
   */
  function onConnectionMonitorServiceTransition (state) {
    mainWindow.webContents.send(CM_IPC_CHANNEL, {
      type: CM_ACTION_TYPES.RENDERER.TRANSITION,
      payload: {
        connectionMonitor: { value: state.value }
      }
    });
  }

  connectionMonitorService.onTransition(onConnectionMonitorServiceTransition);

  /**
   * Start the connection monitor service. All this does is set the monitor
   * to its default state of `DISCONNECTED`
   */
  connectionMonitorService.start();

  /**
   * Get the reference to the spawned ping service using the `PING_SERVICE_ID`
   * constant.
   */
  const pingService = connectionMonitorService.children.get(PING_SERVICE_ID);

  if (!pingService) {
    throw new Error('Connection Monitor Service did not spawn a Ping Service');
  }

  /**
   * Create another onTransition listener for the ping service to forward along
   * state and context to the renderer process.
   */
  function onPingServiceTransition (state) {
    mainWindow.webContents.send(CM_IPC_CHANNEL, {
      type: CM_ACTION_TYPES.RENDERER.TRANSITION,
      payload: {
        ping: { value: state.value }
      }
    });
  }

  pingService.onTransition(onPingServiceTransition);

  /**
   * Set up a reducer for handeling messages coming from the renderer
   * process. This reducer method uses the action types defined in the
   * constants object. This pattern is useful for consolidating how the
   * process communicate about a similar service.
   */
  function connectionMonitorMainProcessReducer (_, action) {
    switch (action.type) {
      case CM_ACTION_TYPES.MAIN.REQUEST_STATE: {
        return {
          connectionMonitor: {
            value: connectionMonitorService.state.value
          },
          ping: {
            value: pingService.state.value
          }
        };
      }
      case CM_ACTION_TYPES.MAIN.TRIGGER_EVENT: {
        connectionMonitorService.send(action.payload);
        break;
      }
      default: {
        console.log(`Unrecognized action type: ${action.type}`);
        break;
      }
    }
  }

  ipcMain.handle(
    CM_IPC_CHANNEL,
    connectionMonitorMainProcessReducer
  );
}

app.whenReady().then(() => {
  createWindow();

  app.on('activate', function () {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit();
});

```

## Guide

The remainder of the guide will breakdown the example into its various core
components. The application utilizes [Interprocess Communication](./process-model.md)
to securely share app state between processes. The core of the example exists
in the main process. The connection monitor and ping machines run on the main
process, and they are interacted with by the user using an API exposed through
the [Context Bridge](./../api/context-bridge.md). The renderer process is built
using HTML, CSS, and ES2020 JavaScript. This can be replaced with any frontend
framework without impacting the core functionality of the example.

### Ping Machine

The `base/ping.js` file defines and exports a function called `createPingMachine`.
This function creates a state machine that can be interpreted as a ping service
for use within the connection monitor. The `options` argument must be passed an
`interval: number`, an `url: string`, and a `window: BrowserWindow` for the
pinging operation. The resulting machine is based on the states and events
defined in `PING_STATES` and `PING_EVENTS`. For this sample, the
`pingOperationService` method uses Node.js [https](https://nodejs.org/api/https.html)
module to make a `HEAD` request to the given `url` endpoint to determine
connection status.

![A state diagram of the ping machine as detailed by this section](../images/connection-monitor-ping-machine-diagram.png)
<!-- https://mermaid-js.github.io/mermaid-live-editor/edit##eyJjb2RlIjoic3RhdGVEaWFncmFtLXYyXG4gICAgWypdIC0tPiBJZGxlXG4gICAgSWRsZSAtLT4gUGluZ2luZyA6IHRvZ2dsZVxuICAgIFBpbmdpbmcgLS0-IFRpbWVvdXQgOiBvbkRvbmUvb25FcnJvclxuICAgIFBpbmdpbmcgLS0-IElkbGUgOiB0b2dnbGVcbiAgICBUaW1lb3V0IC0tPiBQaW5naW5nIDogaW50ZXJ2YWxcbiAgICBUaW1lb3V0IC0tPiBJZGxlIDogdG9nZ2xlXG4gICAgICAgICAgICAiLCJtZXJtYWlkIjoie1xuICBcInRoZW1lXCI6IFwiZGVmYXVsdFwiXG59IiwidXBkYXRlRWRpdG9yIjp0cnVlLCJhdXRvU3luYyI6dHJ1ZSwidXBkYXRlRGlhZ3JhbSI6ZmFsc2V9 -->

Starting in the `IDLE` state, the machine waits for a `TOGGLE` event before
transitioning to the `PINGING` state. At any time, the `TOGGLE` event will
transition this machine back to the `IDLE` state.

Upon entering the `PINGING` state, the machine invokes the
`pingOperationService` method. The service makes a single `HEAD` request to the
given `options.url`. The method returns a `Promise` that _resolves_ when the
request is successful (`statusCode === 200`), and _rejects_ otherwise.
Additionally, if an `'error'` event is emitted from the `request`, and the
`error.code === 'ENOTFOUND'`, a custom error message is rejected and sent to
the renderer process using the `RENDERER.ERROR` action. If the service resolves,
the machine sends a `CONNECT` event to the parent machine; otherwise, if it
rejects, it sends a `DISCONNECT` event to the parent. Regardless, this machine
then transitions to the `TIMEOUT` state.

Upon entering the `TIMEOUT` state, it waits `options.interval` milliseconds
before transitioning back to the `PINGING` state restarting the cycle.

### Connection Monitor Machine

The connection monitor machine is defined in `base/connectionMonitor.js`. Similar to the ping machine, it is instantiated using the `createConnectionMonitorMachine` function and can be interpreted as a service using the XState `interpret` method. The `options` object for this method expects the same arguments as the `options` object for the ping machine since all of the properties are forwarded along. The machine initially instantiates a ping machine instance:

```js
const pingMachine = createPingMachine({
  interval: options.interval || 5000,
  url: options.url,
  window: options.window
})
```

This machine is immediately invoked by the connection monitor machine and is assigned the `PING_SERVICE_ID` constant so that it can be referenced later in the application. The rest of the connection monitor machine defines the various states and events using `CM_STATES` and `CM_EVENTS` constants.

![A state diagram of the Connection Monitor machine described in this section](../images/connection-monitor-machine-diagram.png)
<!--https://mermaid-js.github.io/mermaid-live-editor/edit/#eyJjb2RlIjoic3RhdGVEaWFncmFtLXYyXG4gICAgc3RhdGUgQ29ubmVjdGlvbk1vbml0b3Ige1xuICAgICAgICBbKl0gLS0-IERpc2Nvbm5lY3RlZFxuICAgICAgICBEaXNjb25uZWN0ZWQgLS0-IENvbm5lY3RlZCA6IGNvbm5lY3RcbiAgICAgICAgQ29ubmVjdGVkIC0tPiBEaXNjb25uZWN0ZWQgOiBkaXNjb25uZWN0XG4gICAgfVxuXG4gICAgc3RhdGUgUGluZyB7XG4gICAgICAgIFsqXSAtLT4gSWRsZVxuICAgICAgICBJZGxlIC0tPiBQaW5naW5nIDogdG9nZ2xlXG4gICAgICAgIFBpbmdpbmcgLS0-IFRpbWVvdXQgOiBvbkRvbmUvb25FcnJvclxuICAgICAgICBQaW5naW5nIC0tPiBJZGxlIDogdG9nZ2xlXG4gICAgICAgIFRpbWVvdXQgLS0-IFBpbmdpbmcgOiBpbnRlcnZhbFxuICAgICAgICBUaW1lb3V0IC0tPiBJZGxlIDogdG9nZ2xlXG4gICAgfVxuXG4gICAgQ29ubmVjdGlvbk1vbml0b3IgLS0-IFBpbmcgOiB0b2dnbGVfcGluZ1xuICAgIFBpbmcgLS0-IENvbm5lY3Rpb25Nb25pdG9yIDogY29ubmVjdFxuICAgIFBpbmcgLS0-IENvbm5lY3Rpb25Nb25pdG9yIDogZGlzY29ubmVjdFxuIiwibWVybWFpZCI6IntcbiAgXCJ0aGVtZVwiOiBcImRlZmF1bHRcIlxufSIsInVwZGF0ZUVkaXRvciI6dHJ1ZSwiYXV0b1N5bmMiOnRydWUsInVwZGF0ZURpYWdyYW0iOnRydWV9-->

The connection monitor machine starts in the `DISCONNECTED` state and invokes the ping machine as a service making it enter the `IDLE` state. When the `TOGGLE_PING` event is triggered, the connection monitor sends a `TOGGLE` event to the ping service. When the ping service successfully _connects_ to the `url`, it will send a `CONNECT` event back to the connection monitor. The connection monitor then enters the `CONNECTED` state and the ping service continues to make requests. At any time, the connection monitor can send a `TOGGLE_PING` event to the ping service turning it off or on. The connection monitor will **not** change state when this happens. Finally, if the ping service ever fails to connect to the `url`, it will send a `DISCONNECT` event to the connection monitor. And if the connection monitor is in the `CONNECTED` state it will transition to the `DISCONNECTED` state.

Since the connection monitor will only change state from `CONNECTED` to `DISCONNECTED` using the `DISCONNECT` event (and vice-versa with the `CONNECT` event), the ping service can _connect_ or _disconnect_ as much as it would like, but the connection monitor will not emit a state change. This composition enables a better application design since the remainder of the application only has to listen to the state change of the connection monitor service and respond to those events when they occur.

### Context Bridge `mainAPI`

As mentioned previously, this example utilizes an IPC based secure context isolation to facilitate communication between the main and renderer processes. While the connection monitor runs entirely on the main process, all of its state transitions are broadcasted to the renderer process. Additionally, the api enables the renderer process to initiate certain events for the connection monitor service such as toggling the ping service. The api follows an action/reducer like pattern where all of the methods invoke or send an object with a `type` and a `payload` that is then handeled by the respective process using a reducer function (that generally switches over the various `type` values to decide what to do). The `CM_ACTION_TYPES` constant defines the set of available action types for both process. The entries are sub-mapped by their respective processes. For example, the `MAIN.REQUEST_STATE` action type would be invoked by the renderer process and handeled by the main process.

Akin to many Electron v12+ examples, the main browser window defined in `main.js` has a `webPreferences.preload` script aptly named `preload.js`. 

```js
contextBridge.exposeInMainWorld('mainAPI', {
  // ...
})
```

This script adds the `mainAPI` to the `window` object on the renderer process and exposes a handful of useful methods. These methods make use of the `MAIN` set of action types.

#### `mainAPI.requestState`

The `requestState` method gets the current state of the connection monitor service.

```js
{
  // ...
  requestState: () =>
    ipcRenderer.invoke(CM_IPC_CHANNEL, {
      type: CM_ACTION_TYPES.MAIN.REQUEST_STATE
    })
  // ...
}
```

It is useful for when the app first starts and the renderer process needs to know what the current state of the main process is. It invokes the `MAIN.REQUEST_STATE` action and can expect to recieve the current state of both the connection monitor and ping services in the form of:

```ts
{
  connectionMonitor?: { value: string },
  ping?: { value: string }
}
```

#### `mainAPI.togglePing`

The `togglePing` method toggles the ping operation. 

```js
{
  // ...
  togglePing: () =>
    ipcRenderer.invoke(CM_IPC_CHANNEL, {
      type: CM_ACTION_TYPES.MAIN.TRIGGER_EVENT,
      payload: CM_EVENTS.TOGGLE_PING
    })
  // ...
}
```

The underlying ping service is either on or off; this method switches between the two states by having the connection monitor send a `TOGGLE_PING` event to ping service. Remember that this will have no affect on the current state of the connection monitor. This method invokes the `MAIN.TRIGGER_EVENT` action with a payload of `CM_EVENTS.TOGGLE_PING`.

#### `mainAPI.addConnectionMonitorListener`

The `addConnectionMonitorListener` method is used to establish a main process to renderer process communication pathway.

```js
{
  // ...
  addConnectionMonitorListener: (listener) => {
    ipcRenderer.on(CM_IPC_CHANNEL, listener)
    return () => {
      ipcRenderer.removeListener(CM_IPC_CHANNEL, listener)
    }
  }
  // ...
}
```

The listener function passed to this method can expect actions with a similar `type` and `payload` to be passed as the second argument. The action types for this method will use the `RENDERER` constants from the action type constant previously mentioned. The renderer process has access to these constant values through the `mainAPI.constants` object defined next.

#### `mainAPI.constants`

The `constants` property contains three constant string maps useful for the renderer process, `CM_STATES`, `PING_STATES`, and `CM_ACTION_TYPES`. In a more robust application these would most likely be bundled directly with the renderer process code, but for this example they are shared via the `mainAPI`.

```js
{
  // ...
  constants: {
    CM_STATES,
    PING_STATES,
    CM_ACTION_TYPES
  }
}
```

### Main Process

The main process portion of this example is comprised entirely within the `createWindow()` method defined in `main.js`. It begins by instantiating a connection monitor machine using the `createConnectionMonitorMachine` method.

```js
const connectionMonitorMachine = createConnectionMonitorMachine({
  url: 'https://login.microsoftonline.com',
  window: mainWindow
})
```

Once instantiated, the machine is interpreted as a service using the provided `interpret` method from XState.

```js
const connectionMonitorService = interpret(connectionMonitorMachine)
```

An `onTransition` handler is added and then the service is started, kicking off the connection monitor initial steps of spawning a ping service and moving into the **DISCONNECTED** state. This listener utilizes the `RENDERER.TRANSITION` action type to share the connection monitor's state as it changes.

```js
function onConnectionMonitorServiceTransition (state) {
  mainWindow.webContents.send(CM_IPC_CHANNEL, {
    type: CM_ACTION_TYPES.RENDERER.TRANSITION,
    payload: {
      connectionMonitor: { value: state.value }
    }
  })
}

connectionMonitorService.onTransition(onConnectionMonitorServiceTransition)

connectionMonitorService.start()
```

Once spawned, the ping service is referenced using the constant `PING_SERVICE_ID` string and a similar `onTransition` handler is established.

```js
const pingService = connectionMonitorService.children.get(PING_SERVICE_ID)

if (!pingService) {
  throw new Error('Connection Monitor Service did not spawn a Ping Service')
}

function onPingServiceTransition (state) {
  mainWindow.webContents.send(CM_IPC_CHANNEL, {
    type: CM_ACTION_TYPES.RENDERER.TRANSITION,
    payload: {
      ping: { value: state.value }
    }
  })
}

pingService.onTransition(onPingServiceTransition)
```

Finally, a main process reducer is defined and hooked up to the constant `CM_IPC_CHANNEL` handler

```js
function connectionMonitorMainProcessReducer (_, action) {
  switch (action.type) {
    case CM_ACTION_TYPES.MAIN.REQUEST_STATE: {
      return {
        connectionMonitor: {
          value: connectionMonitorService.state.value
        },
        ping: {
          value: pingService.state.value
        }
      }
    }
    case CM_ACTION_TYPES.MAIN.TRIGGER_EVENT: {
      connectionMonitorService.send(action.payload)
      break
    }
    default: {
      console.log(`Unrecognized action type: ${action.type}`)
      break
    }
  }
}

ipcMain.handle(
  CM_IPC_CHANNEL,
  connectionMonitorMainProcessReducer
)
```

### Renderer Process

The final part of this example is the renderer process. As mentioned previously, it uses HTML, CSS, and ES2020 JavaScript and could be replaced by any frontend framework. The `mainAPI` defined in `preload.js` is available on the global `window` object and used to interact with the main process. The file has a collection of code blocks responsible for updating the UI in response to changes from the main process. The rendering is mainly controlled by two parts.

First, the listener that utilizes the `mainAPI.addConnectionMonitorListener` method to sync with the main process. As defined in the [Context Bridge `mainAPI`](#context-bridge-mainapi) section, this method expects action objects and uses a `switch` statement to handle the different types. The `ERROR` type is used to send connection monitor error messages with the UI, and the `TRANSITION` type is used to initiate a page update when the state of either main process services change.

```js
function connectionMonitorListener (_, action) {
  const { CM_ACTION_TYPES } = window.mainAPI.constants
  switch (action.type) {
    case CM_ACTION_TYPES.RENDERER.ERROR: {
      updateError(action.payload)
      break
    }
    case CM_ACTION_TYPES.RENDERER.TRANSITION: {
      updatePage(action.payload)
      break
    }
    default:
      console.log(`Unrecognized action ${action.type}`)
  }
}

window.mainAPI.addConnectionMonitorListener(connectionMonitorListener)
```

Second, a `window.onload` method is defined to request the initial state of the main process services so the UI can render respectively. This function is important because the services may start executing before the page finishes loading. Additionally, this kind of method is helpful for fetching the initial state when only the renderer process is reloaded and the main process remains the same.

```js
window.onload = () => {
  window.mainAPI.requestState().then((states) => {
    updatePage(states)
  })
}
```

Finally, at the end of the renderer process code, a series of DOM-related methods are defined for modifying the UI based on the service updates.

```js
function updatePage (states) {
  if (states.connectionMonitor) {
    updateConnectionMonitorElements(states.connectionMonitor)
  }

  if (states.ping) {
    updatePingStatus(states.ping)
  }
}

function updatePingStatus ({ value }) {
  const { PING_STATES } = window.mainAPI.constants
  const pingStatus = document.getElementById('ping-status')
  pingStatus.innerHTML = value === PING_STATES.TIMEOUT ? 'pinging' : value
}

function updateConnectionMonitorElements ({ value }) {
  const connectionStatus = document.getElementById('connection-status')
  connectionStatus.innerHTML = value
}

function updateError ({ error }) {
  document.getElementById('error').hidden = false
  document.getElementById('error-message').innerHTML = error.message
}

document.getElementById('toggle-ping').onclick =
  function onPingButtonClick () {
    window.mainAPI.togglePing()
  }

document.getElementById('clear-error').onclick =
  function onClearErrorButtonClick () {
    document.getElementById('error-message').innerHTML = ''
    document.getElementById('error').hidden = true
  }
```

###

## Conclusion

This guide introduces a connection monitoring service for Electron applications. As the authentication example demonstrates, the state machines are adaptable to fit your project's specific needs as well.
