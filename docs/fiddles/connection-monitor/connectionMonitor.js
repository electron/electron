const { createMachine, send, assign } = require("xstate");
const msal = require("@azure/msal-node");
const { BrowserWindow } = require("electron");
const {
  CONNECTION_MONITOR_IPC_CHANNEL,
  CONNECTION_MONITOR_STATES: STATES,
  CONNECTION_MONITOR_EVENTS: EVENTS,
  PING_EVENTS,
  PING_SERVICE_ID,
  CONNECTION_MONITOR_IPC_REDUCER_ACTION_TYPES: ACTION_TYPES
} = require("./constants.js");
const { createPingMachine } = require("./ping.js");

function authenticate(msalApp, redirectUri) {
  return new Promise((resolve, reject) => {
    const window = new BrowserWindow({ width: 400, height: 600 });
    const closeListener = () => {
      reject(new Error("Auth flow cancelled"));
    };
    window.on("close", closeListener);
    const cryptoProvider = new msal.CryptoProvider();
    cryptoProvider
      .generatePkceCodes()
      .then(({ challenge, verifier }) => {
        msalApp
          .getAuthCodeUrl({
            scopes: ["user.read"],
            redirectUri,
            codeChallenge: challenge,
            codeChallengeMethod: "S256",
          })
          .then((authCodeURL) => {
            window.loadURL(authCodeURL);
            window.webContents.on("will-redirect", (_, responseURL) => {
              const parsedUrl = new URL(responseURL);
              const authCode = parsedUrl.searchParams.get("code");
              if (authCode) {
                msalApp
                  .acquireTokenByCode({
                    redirectUri,
                    scopes: ["user.read"],
                    code: authCode,
                    codeVerifier: verifier,
                  })
                  .then((authResult) => {
                    window.removeListener("close", closeListener);
                    window.close();
                    resolve(authResult);
                  })
                  .catch(reject);
              }
            });
          })
          .catch(reject);
      })
      .catch(reject);
  });
}

function createConnectionMonitorMachine(options) {
  if (!options.window) {
    throw new Error(
      `ConnectionMonitor options is missing required property 'window'`
    );
  }

  if (!options.msal) {
    throw new Error(
      `ConnectionMonitor options is missing required property 'msal'`
    );
  }

  const msalApp = new msal.PublicClientApplication({
    auth: options.msal.auth,
    system: {
      loggerOptions: {
        loggerCallback: console.log,
        piiLoggingEnabled: false,
        logLevel: msal.LogLevel.Info,
      },
    },
  });

  const pingMachine = createPingMachine({
    interval: options.interval || 5000,
    url: "https://login.microsoftonline.com",
  });

  return createMachine({
    id: "connectionMonitor",
    context: { user: null },
    initial: STATES.DISCONNECTED,
    invoke: { id: PING_SERVICE_ID, src: pingMachine },
    states: {
      [STATES.DISCONNECTED]: {
        on: {
          [EVENTS.TOGGLE_PING]: {
            actions: send({ type: PING_EVENTS.TOGGLE }, { to: PING_SERVICE_ID }),
          },
          [EVENTS.CONNECT]: {
            target: STATES.CONNECTED,
          },
        },
      },
      [STATES.CONNECTED]: {
        on: {
          [EVENTS.TOGGLE_PING]: {
            actions: send({ type: PING_EVENTS.TOGGLE }, { to: PING_SERVICE_ID }),
          },
          [EVENTS.DISCONNECT]: {
            target: STATES.DISCONNECTED,
          },
          [EVENTS.TOGGLE_AUTH]: {
            target: STATES.AUTHENTICATING,
          },
        },
      },
      [STATES.AUTHENTICATING]: {
        invoke: {
          id: "authenticateService",
          src: () => authenticate(msalApp, options.msal.redirectUri),
          onDone: {
            target: STATES.AUTHENTICATED,
            actions: assign({ user: (_, event) => event.data }),
          },
          onError: {
            target: STATES.CONNECTED,
            actions: (_, event) => {
              options.window.webContents.send(CONNECTION_MONITOR_IPC_CHANNEL, {
                type: ACTION_TYPES.RENDERER.ERROR,
                payload: {
                  error: event.data,
                },
              });
            },
          },
        },
      },
      [STATES.AUTHENTICATED]: {
        on: {
          [EVENTS.TOGGLE_PING]: {
            actions: send({ type: PING_EVENTS.TOGGLE }, { to: PING_SERVICE_ID }),
          },
          [EVENTS.DISCONNECT]: {
            target: STATES.DISCONNECTED,
          },
          [EVENTS.TOGGLE_AUTH]: {
            target: STATES.CONNECTED,
            actions: assign({ user: null }),
          },
        },
      },
    },
  });
}
module.exports = {
  createConnectionMonitorMachine,
};
