var EventEmitter, PERSIST_PERFIX, bindings, wrapSession;

EventEmitter = require('events').EventEmitter;

bindings = process.atomBinding('session');

PERSIST_PERFIX = 'persist:';

// Returns the Session from |partition| string.
exports.fromPartition = function(partition) {
  if (partition == null) {
    partition = '';
  }
  if (partition === '') {
    return exports.defaultSession;
  }
  if (partition.startsWith(PERSIST_PERFIX)) {
    return bindings.fromPartition(partition.substr(PERSIST_PERFIX.length), false);
  } else {
    return bindings.fromPartition(partition, true);
  }
};

// Returns the default session.
Object.defineProperty(exports, 'defaultSession', {
  enumerable: true,
  get: function() {
    return bindings.fromPartition('', false);
  }
});

wrapSession = function(session) {
  // session is an EventEmitter.
  return session.__proto__ = EventEmitter.prototype;
};

bindings._setWrapSession(wrapSession);
