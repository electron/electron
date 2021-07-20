const { createMachine, sendParent } = require('xstate');
const https = require('https');
const {
  PING_STATES,
  PING_EVENTS,
  CM_EVENTS,
  CM_IPC_CHANNEL,
  CM_ACTION_TYPES
} = require('./constants.js');

/**
 * This function is the core of the ping machine. It is a Node.js https.request
 * call wrapped in a promise. The request itself uses the `HEAD` method to keep
 * resource consumption low. The internals of this method can be modified as
 * long as `resolve` means "connect" and `reject` means "disconnect" for the
 * machine.
 *
 * For the purposes of this example, this method only resolves when it recieves
 * a 200 response from the url. Additionally, it has a custom error message
 * when it encounters an ENOTFOUND error code (usually indicating the machine
 * cannot access the internet).
 */
function pingOperationService (url) {
  return new Promise((resolve, reject) => {
    const request = https.request(url, { method: 'HEAD' }, (response) => {
      if (response.statusCode === 200) {
        resolve();
      } else {
        // eslint-disable-next-line prefer-promise-reject-errors
        reject();
      }
    });

    request.on('error', error => {
      if (error.code === 'ENOTFOUND') {
        reject(new Error(`Cannot connect to ${url}. Is your machine online?`));
      } else {
        reject(error);
      }
    });

    request.end();
  });
}

/**
 * Create a ping machine for use within the connection monitor service.
 * Options must be passed an `interval: number`, an `url: string`, and
 * `window: BrowserWindow`. The resulting machine is based on the relative
 * constant string maps `PING_STATES` and `PING_EVENTS`. For this sample,
 * the `pingService` action uses Node.js https module to make a `HEAD`
 * request to the given `url` endpoint to determine connection status.
 */
function createPingMachine (options) {
  if (!options.interval) {
    throw new Error(
      "Ping machine options missing required property 'interval'"
    );
  }
  if (!options.url) {
    throw new Error("Ping machine options missing required property 'url'");
  }
  if (!options.window) {
    throw new Error("Ping machine options missing required property 'window'");
  }

  return createMachine({
    id: 'pingMachine',
    initial: PING_STATES.IDLE,
    states: {
      /*
       * Starting in the `IDLE` state, the machine waits for a `TOGGLE`
       * event before transitioning to the `PINGING` state. At any time,
       * the `TOGGLE` event will transition this machine back to the `IDLE`
       * state.
       */
      [PING_STATES.IDLE]: {
        on: {
          [PING_EVENTS.TOGGLE]: PING_STATES.PINGING
        }
      },
      /*
       * Upon entering the `PINGING` state, the machine invokes a service
       * that makes a single `HEAD` request to the given `options.url`. If
       * this request resolves with a 200 statusCode, the machine sends a
       * `CONNECT` event to the parent machine; otherwise, it sends a
       * `DISCONNECT` event to the parent. Regardless, this machine then
       * transitions to the `TIMEOUT` state.
       */
      [PING_STATES.PINGING]: {
        on: {
          [PING_EVENTS.TOGGLE]: PING_STATES.IDLE
        },
        invoke: {
          id: 'pingService',
          src: () => pingOperationService(options.url),
          onDone: {
            actions: sendParent(CM_EVENTS.CONNECT),
            target: PING_STATES.TIMEOUT
          },
          onError: {
            actions: [
              sendParent(CM_EVENTS.DISCONNECT),
              (_, event) => {
                if (event.data instanceof Error) {
                  options.window.webContents.send(CM_IPC_CHANNEL, {
                    type: CM_ACTION_TYPES.RENDERER.ERROR,
                    payload: {
                      error: event.data
                    }
                  });
                }
              }
            ],
            target: PING_STATES.TIMEOUT
          }
        }
      },
      /**
       * Upon entering the `TIMEOUT` state, it waits `options.interval`
       * milliseconds before transitioning back to the `PINGING` state,
       * restarting this cycle.
       */
      [PING_STATES.TIMEOUT]: {
        on: {
          [PING_EVENTS.TOGGLE]: PING_STATES.IDLE
        },
        after: {
          [options.interval]: { target: PING_STATES.PINGING }
        }
      }
    }
  });
}

module.exports = {
  createPingMachine
};
