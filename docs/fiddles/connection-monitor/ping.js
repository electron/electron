const { createMachine, sendParent } = require('xstate');
const https = require('https');
const {
  PING_STATES,
  PING_EVENTS,
  CONNECTION_MONITOR_EVENTS
} = require('./constants.js');

/**
 * This function is the core of the Ping Machine. It is a Node.js https.request
 * call wrapped in a promise. The request itself uses the `HEAD` method to keep
 * resource consumption low. The internals of this method can be modified as
 * long as `resolve` means "connect" and `reject` means "disconnect" for the
 * machine.
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
    request.end();
  });
}

/**
 * Create a ping machine for use within the Connection Monitor.
 * Options must be passed an `interval: number` and an `url: string` for
 * the pinging operation. The resulting machine is based on the relative
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

  return createMachine({
    id: 'pingMachine',
    initial: PING_STATES.IDLE,
    states: {
      /*
       * Starting in the **IDLE** state, the machine waits for a **TOGGLE**
       * event before transitioning to the **PINGING** state. At any time,
       * the **TOGGLE** event will transition this machine back to the **IDLE**
       * state.
       */
      [PING_STATES.IDLE]: {
        on: {
          [PING_EVENTS.TOGGLE]: PING_STATES.PINGING
        }
      },
      /*
       * Upon entering the **PINGING** state, the machine invokes a service
       * that makes a single `HEAD` request to the given `options.url`. If
       * this request resolves with a 200 statusCode, the machine sends a
       * **CONNECT** event to the parent machine; otherwise, it sends a
       * **DISCONNET** event to the parent. Regardless, this machine then
       * transitions to the **TIMEOUT** state.
       */
      [PING_STATES.PINGING]: {
        on: {
          [PING_EVENTS.TOGGLE]: PING_STATES.IDLE
        },
        invoke: {
          id: 'pingService',
          src: () => pingOperationService(options.url),
          onDone: {
            actions: sendParent(CONNECTION_MONITOR_EVENTS.CONNECT),
            target: PING_STATES.TIMEOUT
          },
          onError: {
            actions: sendParent(CONNECTION_MONITOR_EVENTS.DISCONNECT),
            target: PING_STATES.TIMEOUT
          }
        }
      },
      /**
       * Upon entering the **TIMEOUT** state, it waits `options.interval`
       * milliseconds before transitioning back to the **PINGING** state
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
