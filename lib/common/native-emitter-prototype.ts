import { EventEmitter } from 'events';

const { setEventEmitterPrototype, setEventObserved, clearObservedEvents, observeAllEvents } = process._linkedBinding(
  'electron_browser_event_emitter'
);

// Native code keeps, for each native emitter, the names of the events that
// have listeners, and does not emit the rest. That set is kept up to date from
// here: this is the prototype every native emitter inherits from, in front of
// EventEmitter.prototype, and whatever adds or removes a listener goes through
// it. once() and prependOnceListener() are Node's own; they call on() and
// prependListener() on the emitter.
const { on, prependListener, removeListener, removeAllListeners } = EventEmitter.prototype;

type Listener = (...args: any[]) => void;

const observe = (emitter: EventEmitter, eventName: string | symbol) => {
  // Native code only emits events with string names.
  if (typeof eventName === 'string') setEventObserved(emitter, eventName, true);
};

const unobserve = (emitter: EventEmitter, eventName: string | symbol) => {
  if (typeof eventName === 'string' && emitter.listenerCount(eventName) === 0) {
    setEventObserved(emitter, eventName, false);
  }
};

const methods = {
  on(this: EventEmitter, eventName: string | symbol, listener: Listener) {
    on.call(this, eventName, listener);
    observe(this, eventName);
    return this;
  },
  prependListener(this: EventEmitter, eventName: string | symbol, listener: Listener) {
    prependListener.call(this, eventName, listener);
    observe(this, eventName);
    return this;
  },
  removeListener(this: EventEmitter, eventName: string | symbol, listener: Listener) {
    removeListener.call(this, eventName, listener);
    unobserve(this, eventName);
    return this;
  },
  removeAllListeners(this: EventEmitter, ...args: [] | [string | symbol]) {
    removeAllListeners.apply(this, args);
    if (args.length === 0) {
      clearObservedEvents(this);
    } else {
      unobserve(this, args[0]);
    }
    return this;
  }
};

const prototype = Object.create(EventEmitter.prototype, {
  on: { value: methods.on, writable: true, configurable: true },
  addListener: { value: methods.on, writable: true, configurable: true },
  prependListener: { value: methods.prependListener, writable: true, configurable: true },
  removeListener: { value: methods.removeListener, writable: true, configurable: true },
  off: { value: methods.removeListener, writable: true, configurable: true },
  removeAllListeners: { value: methods.removeAllListeners, writable: true, configurable: true },
  // An emit() of its own sees every event an emitter has, listeners or not:
  // utilityProcess and powerMonitor route their native handle's events that
  // way. Assigning one tells native code to stop skipping for that emitter.
  emit: {
    configurable: true,
    get() {
      return EventEmitter.prototype.emit;
    },
    set(this: EventEmitter, value: unknown) {
      Object.defineProperty(this, 'emit', { value, writable: true, enumerable: true, configurable: true });
      observeAllEvents(this);
    }
  }
});

setEventEmitterPrototype(prototype);
