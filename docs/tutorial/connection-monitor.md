# Connection Monitor

Many applications require some sort of online/offline detection mechanism.
As detailed in the [Online/Offline Event Detection](./online-offline-events.md)
guide, the chromium browser does provide a mechanism for determining
connectivity; however, this mechanism is trivial. It only checks for the
presance of a _network_ connection on the machine and not if it can actually
connect to a given resource. This example builds a connection monitoring
service that pings a given resource over a given interval. Depending on the
result of that request, the service changes its state from **CONNECTED** to
**DISCONNECTED** and shares this information with the rest of application. In
addition, the same service is wired up to a common authentication library,
[@azure/msal-node](https://www.npmjs.com/package/@azure/msal-node) to
demonstrate how the service can be expanded to support a production-ready
authentication flow.

This example was built with the intent to be a starting point for other
projects. You'll most likely have to changes things to suit your own needs.
Certain parts of the example were designed for this exact reason. For example,
the `authenticationOperationService` and the `pingOperationService` functions
are generalized in a way where you may only need to modify the internal logic
of the function and the state machine can remain the same (more details later).

Additionally, this example is built on the
[XState](https://github.com/statelyai/xstate) library. The connection monitor
service is a state machine and utilizes the XState API heavily. Knowledge of
state machines is not required to follow along with this example, but is
encouraged. For more information as well as an useful introduction to state
machines, read through the [XState documentation](https://xstate.js.org/docs/).

Before getting started with the example, if you intend on running this locally
with `@azure/msal-node` you'll need to include some environment variables. The
following section will walk you through everything you need to do.

## Configuring `@azure/msal-node`

> If you are using your own authentication library you may skip this section

In order to run this example locally you'll need an Azure AAD Tenant as well as
an Azure App Registration. This guide on [Registering your app](https://docs.microsoft.com/en-us/graph/auth-register-app-v2)
will show you how to do so. You'll need to keep note of 3 things:

1. The App Registration Client ID
2. Your AAD Tenant Authority (for a majority of users this will be
`https://login.microsoft.com/common`)
3. The app authentication redirect URI

When creating the redirect URI, select "Public Client (Mobile & Desktop)". The
URI should be in the form `msal<APP_CLIENT_ID>://auth`.

Assign the three values to their respective environment variables:

```sh
MSAL_CLIENT_ID=
MSAL_AUTHORITY=
MSAL_REDIRECT_URI=
```

Or in-line them in the options object passed to the
`createConnectionMonitorMachine` function call in `main.js`.

## Example

```javascript fiddle='docs/fiddles/connection-monitor/main.js'
const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const { interpret } = require('xstate');
const { createConnectionMonitorMachine } = require('./connectionMonitor.js');

/*
 * Either create a .env file and add the necessary variables as described in
 * the set up steps, or replace `process.env.MSAL_*` variables with the
 * respective strings in-line.
 */
require('dotenv').config();

const {
  CONNECTION_MONITOR_IPC_CHANNEL,
  PING_SERVICE_ID,
  CONNECTION_MONITOR_IPC_REDUCER_ACTION_TYPES: ACTION_TYPES
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
    window: mainWindow,
    msal: {
      auth: {
        clientId: process.env.MSAL_CLIENT_ID,
        authority: process.env.MSAL_AUTHORITY
      },
      redirectUri: process.env.MSAL_REDIRECT_URI
    }
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
    mainWindow.webContents.send(CONNECTION_MONITOR_IPC_CHANNEL, {
      type: ACTION_TYPES.RENDERER.TRANSITION,
      payload: {
        connectionMonitor: { value: state.value, context: state.context }
      }
    });
  }

  connectionMonitorService.onTransition(onConnectionMonitorServiceTransition);

  /**
   * Start the connection monitor service. All this does is set the monitor
   * to its default state of **DISCONNECTED**
   */
  connectionMonitorService.start();

  /**
   * Get the reference to the spawned Ping service using the `PING_SERVICE_ID`
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
    mainWindow.webContents.send(CONNECTION_MONITOR_IPC_CHANNEL, {
      type: ACTION_TYPES.RENDERER.TRANSITION,
      payload: {
        ping: { value: state.value }
      }
    });
  }

  pingService.onTransition(onPingServiceTransition);

  /**
   * Set up a reducer for for handeling messages coming from the renderer
   * process. This reducer method uses the action types defined in the
   * constants object. This pattern is useful for consolidating how the
   * process communicate about a similar service.
   */
  function connectionMonitorMainProcessReducer (_, action) {
    switch (action.type) {
      case ACTION_TYPES.MAIN.REQUEST_INIT_STATE: {
        return {
          connectionMonitor: {
            value: connectionMonitorService.state.value,
            context: connectionMonitorService.state.context
          }
        };
      }
      case ACTION_TYPES.MAIN.TRIGGER_EVENT: {
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
    CONNECTION_MONITOR_IPC_CHANNEL,
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
