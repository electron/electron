// Modules to control application life and create native browser window
const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const { interpret } = require('xstate');
const { createConnectionMonitorMachine } = require('./connectionMonitor.js');
require('dotenv').config();

const {
  CONNECTION_MONITOR_IPC_CHANNEL,
  PING_SERVICE_ID,
  CONNECTION_MONITOR_IPC_REDUCER_ACTION_TYPES: ACTION_TYPES
} = require('./constants.js');

function createWindow () {
  // Create the browser window.
  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  });

  // and load the index.html of the app.
  mainWindow.loadFile('index.html');

  // Open the DevTools.
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

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(() => {
  createWindow();

  app.on('activate', function () {
    // On macOS it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit();
});

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.
