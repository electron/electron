/*
 * A lot of things in XState are key-based, and without TypeScript it
 * can get confusing where things are going wrong. By using enum-like
 * string maps, as long as the implementations are using these constant
 * objects, it reduces the chance of things breaking because of a spelling
 * error or other common developer oversights.
 */

/**
 * The IPC Channel all Connection Monitor data is transmitted through.
 * The implementation uses an action/reducer like API to handle the
 * different things coming over the channel.
 */
const CONNECTION_MONITOR_IPC_CHANNEL = 'connection-monitor ipc channel';

/**
 * The service ID for the Ping service the Connection Monitor spawns.
 * This is useful for getting the service later for hooking up state
 * transition listeners.
 */
const PING_SERVICE_ID = 'pingService';

/**
 * A string map of Connection Monitor machine Event keys
 */
const CONNECTION_MONITOR_EVENTS = {
  TOGGLE_PING: 'toggle ping',
  CONNECT: 'connect',
  DISCONNECT: 'disconnect',
  TOGGLE_AUTH: 'toggle auth'
};

/**
 * A string map of Connection Monitor machine State keys
 */
const CONNECTION_MONITOR_STATES = {
  CONNECTED: 'connected',
  DISCONNECTED: 'disconnected',
  AUTHENTICATING: 'authenticating',
  AUTHENTICATED: 'authenticated'
};

/**
 * A string map of Ping machine State keys
 */
const PING_STATES = {
  IDLE: 'idle',
  PINGING: 'pinging',
  TIMEOUT: 'timeout'
};

/**
 * A string map of Ping machine Event keys
 */
const PING_EVENTS = {
  TOGGLE: 'toggle'
};

/**
 * A complex string map for the different action types the
 * reducers on both process will receive.
 */
const CONNECTION_MONITOR_IPC_REDUCER_ACTION_TYPES = {
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
  CONNECTION_MONITOR_IPC_CHANNEL,
  PING_SERVICE_ID,
  CONNECTION_MONITOR_STATES,
  CONNECTION_MONITOR_EVENTS,
  PING_STATES,
  PING_EVENTS,
  CONNECTION_MONITOR_IPC_REDUCER_ACTION_TYPES
};
