/*
 * A lot of things in XState are key-based, and without TypeScript it
 * can get confusing where things are going wrong. By using enum-like
 * string maps, as long as the implementations are using these constant
 * objects, it reduces the chance of things breaking because of a spelling
 * error or other common developer oversights.
 */

/**
 * The IPC Channel all connection monitor data is transmitted through.
 * The implementation uses an action/reducer like API to handle the
 * different things coming over the channel.
 */
const CM_IPC_CHANNEL = 'connection-monitor ipc channel';

/**
 * The service ID for the ping service the connection monitor spawns.
 * This is useful for getting the service later for hooking up state
 * transition listeners.
 */
const PING_SERVICE_ID = 'pingService';

/**
 * A string map of connection monitor machine event keys
 */
const CM_EVENTS = {
  TOGGLE_PING: 'toggle ping',
  CONNECT: 'connect',
  DISCONNECT: 'disconnect'
};

/**
 * A string map of connection monitor machine state keys
 */
const CM_STATES = {
  CONNECTED: 'connected',
  DISCONNECTED: 'disconnected'
};

/**
 * A string map of ping machine state keys
 */
const PING_STATES = {
  IDLE: 'idle',
  PINGING: 'pinging',
  TIMEOUT: 'timeout'
};

/**
 * A string map of ping machine event keys
 */
const PING_EVENTS = {
  TOGGLE: 'toggle'
};

/**
 * A complex string map for the different action types the
 * reducers on both process will receive.
 */
const CM_ACTION_TYPES = {
  MAIN: {
    REQUEST_STATE: 'request state',
    TRIGGER_EVENT: 'trigger event'
  },
  RENDERER: {
    ERROR: 'error',
    TRANSITION: 'transition'
  }
};

module.exports = {
  CM_IPC_CHANNEL,
  PING_SERVICE_ID,
  CM_STATES,
  CM_EVENTS,
  PING_STATES,
  PING_EVENTS,
  CM_ACTION_TYPES
};
