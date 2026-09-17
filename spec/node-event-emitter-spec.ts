import { BrowserWindow, ipcMain } from 'electron/main';

import { expect } from 'chai';

import { EventEmitter as NodeEventEmitter, once } from 'node:events';
import * as path from 'node:path';

import { closeAllWindows } from './lib/window-helpers';

// The native EventEmitter that sandboxed renderers use in place of Node's
// `events` module. It is registered as a common binding, so the main process
// can load it too and compare it against Node's implementation directly.
const { EventEmitter: NativeEventEmitter } = process._linkedBinding('electron_common_events');

type Impl = typeof NodeEventEmitter;

// Runs `scenario` against both implementations and asserts that everything it
// recorded is identical.
function sameAsNode(scenario: (EventEmitter: Impl, log: (...values: any[]) => void) => void) {
  const run = (impl: Impl) => {
    const log: any[] = [];
    try {
      scenario(impl, (...values: any[]) => log.push(values));
    } catch (error: any) {
      log.push(['threw', error?.name, error?.message]);
    }
    return log;
  };
  const expected = run(NodeEventEmitter);
  const actual = run(NativeEventEmitter as unknown as Impl);
  expect(actual).to.deep.equal(expected);
  // Guard against scenarios that silently record nothing.
  expect(expected.length).to.be.greaterThan(0);
}

function captureThrow(fn: () => unknown): any {
  try {
    fn();
  } catch (error) {
    return error;
  }
  expect.fail('expected function to throw');
}

describe('native EventEmitter (electron_common_events)', () => {
  it("is a distinct implementation from Node's", () => {
    expect(NativeEventEmitter).to.not.equal(NodeEventEmitter);
    expect(NativeEventEmitter.prototype).to.not.equal(NodeEventEmitter.prototype);
  });

  describe('shape', () => {
    it('has the same prototype property names as Node', () => {
      expect(Object.getOwnPropertyNames(NativeEventEmitter.prototype).sort()).to.deep.equal(
        Object.getOwnPropertyNames(NodeEventEmitter.prototype).sort()
      );
    });

    it('has the same enumerable prototype keys as Node, in the same order', () => {
      expect(Object.keys(NativeEventEmitter.prototype)).to.deep.equal(Object.keys(NodeEventEmitter.prototype));
    });

    it('has matching method names and lengths', () => {
      for (const key of Object.getOwnPropertyNames(NodeEventEmitter.prototype)) {
        // Node's constructor takes an options bag for captureRejections.
        if (key === 'constructor') continue;
        const nodeValue = (NodeEventEmitter.prototype as any)[key];
        const nativeValue = (NativeEventEmitter.prototype as any)[key];
        expect(typeof nativeValue).to.equal(typeof nodeValue, key);
        if (typeof nodeValue === 'function') {
          expect(nativeValue.name).to.equal(nodeValue.name, `${key}.name`);
          expect(nativeValue.length).to.equal(nodeValue.length, `${key}.length`);
        } else {
          expect(nativeValue).to.equal(nodeValue, key);
        }
      }
    });

    it('aliases on/addListener and off/removeListener', () => {
      expect(NativeEventEmitter.prototype.on).to.equal(NativeEventEmitter.prototype.addListener);
      expect(NativeEventEmitter.prototype.off).to.equal(NativeEventEmitter.prototype.removeListener);
    });

    it('only exposes defaultMaxListeners on the class', () => {
      expect(NativeEventEmitter.name).to.equal('EventEmitter');
      expect(NativeEventEmitter.defaultMaxListeners).to.equal(10);
      expect(Object.getOwnPropertyNames(NativeEventEmitter).sort()).to.deep.equal([
        'defaultMaxListeners',
        'length',
        'name',
        'prototype'
      ]);
    });

    it('gives instances the same own properties as Node', () => {
      sameAsNode((EventEmitter, log) => {
        const e = new EventEmitter();
        log(
          Object.getOwnPropertyNames(e),
          (e as any)._eventsCount,
          (e as any)._maxListeners,
          Object.getPrototypeOf((e as any)._events)
        );
      });
    });
  });

  describe('behaves like Node', () => {
    it('for emit ordering, arguments, this and return values', () => {
      sameAsNode((EventEmitter, log) => {
        const e = new EventEmitter();
        log(e.emit('nothing'));
        e.on('a', function (this: any, ...args: any[]) {
          log('a1', this === e, args);
        });
        e.on('a', (...args: any[]) => log('a2', args));
        e.prependListener('a', () => log('a0'));
        log(e.emit('a', 1, 'two', { three: 3 }));
        log(e.listenerCount('a'), e.eventNames());
      });
    });

    it('for once, prependOnceListener and listener unwrapping', () => {
      sameAsNode((EventEmitter, log) => {
        const e = new EventEmitter();
        const fn = () => log('once');
        e.once('x', fn);
        e.prependOnceListener('x', () => log('first'));
        log(
          e.listeners('x').map((l) => l === fn),
          e.rawListeners('x').map((l: any) => l === fn || l.listener === fn)
        );
        log(e.listenerCount('x'));
        e.emit('x');
        e.emit('x');
        log(e.listenerCount('x'), (e as any)._eventsCount);
      });
    });

    it('for removeListener matching, order and once wrappers', () => {
      sameAsNode((EventEmitter, log) => {
        const e = new EventEmitter();
        const a = () => log('a');
        const b = () => log('b');
        e.on('x', a).on('x', b).on('x', a).once('x', b);
        e.removeListener('x', a);
        log(e.listeners('x').map((l) => (l === a ? 'a' : 'b')));
        e.removeListener('x', b);
        log(e.listeners('x').map((l) => (l === a ? 'a' : 'b')));
        e.emit('x');
        log(e.removeListener('nope', a) === e, e.off === e.removeListener);
        e.removeAllListeners('x');
        log(e.eventNames(), (e as any)._eventsCount);
      });
    });

    it('for newListener and removeListener meta events', () => {
      sameAsNode((EventEmitter, log) => {
        const e = new EventEmitter();
        const fn = () => {};
        e.on('newListener', (name: any, listener: any) => log('new', name, listener === fn, e.listenerCount(name)));
        e.on('removeListener', (name: any, listener: any) =>
          log('removed', name, listener === fn, e.listenerCount(name))
        );
        e.on('x', fn);
        e.once('x', fn);
        e.prependListener('y', fn);
        e.removeListener('x', fn);
        e.removeAllListeners('x');
        e.removeAllListeners();
        log(e.eventNames());
      });
    });

    it('for listeners added or removed during emit', () => {
      sameAsNode((EventEmitter, log) => {
        const e = new EventEmitter();
        const late = () => log('late');
        const second = () => log('second');
        e.on('x', () => {
          log('first');
          e.on('x', late);
          e.removeListener('x', second);
        });
        e.on('x', second);
        e.emit('x');
        log('--');
        e.emit('x');
        log(e.rawListeners('x').length);
      });
    });

    it('when a listener throws (later listeners are skipped, emit rethrows)', () => {
      sameAsNode((EventEmitter, log) => {
        const e = new EventEmitter();
        const boom = new Error('boom');
        e.on('one', () => {
          log('one:first');
          throw boom;
        });
        try {
          e.emit('one', 1);
        } catch (error) {
          log('caught', error === boom);
        }
        e.on('x', () => log('first'));
        e.on('x', () => {
          log('second');
          throw boom;
        });
        e.on('x', () => log('third'));
        try {
          e.emit('x');
        } catch (error) {
          log('caught', error === boom);
        }
        log(e.listenerCount('x'), e.listenerCount('one'));
        e.removeAllListeners('x');
        e.on('x', () => log('again'));
        log(e.emit('x'));
      });
    });

    it('for an unhandled error event with an Error', () => {
      sameAsNode((EventEmitter, log) => {
        const e = new EventEmitter();
        const err = new TypeError('boom');
        try {
          e.emit('error', err);
        } catch (error: any) {
          log(error === err, error.name, error.message);
        }
        e.on('error', (handled: any) => log('handled', handled === err));
        log(e.emit('error', err));
      });
    });

    it('for max listeners', () => {
      sameAsNode((EventEmitter, log) => {
        const e = new EventEmitter();
        log(e.getMaxListeners(), e.setMaxListeners(2) === e, e.getMaxListeners());
        e.setMaxListeners(0);
        log(e.getMaxListeners());
      });
    });

    it('for symbol, numeric and object event names', () => {
      sameAsNode((EventEmitter, log) => {
        const e = new EventEmitter();
        const sym = Symbol('s');
        const obj = { toString: () => 'obj-key' };
        const numListener = () => log('num');
        const objListener = () => log('obj');
        e.on(sym, () => log('sym'));
        e.on(42 as any, numListener);
        e.on(obj as any, objListener);
        e.emit(sym);
        e.emit(42 as any);
        e.emit('42');
        e.emit('obj-key');
        log(e.eventNames().map((n) => (typeof n === 'symbol' ? n.toString() : n)));
        e.removeListener(42 as any, numListener);
        e.removeListener(obj as any, objListener);
        log(e.eventNames().map((n) => (typeof n === 'symbol' ? n.toString() : n)));
        e.removeAllListeners();
        log(e.eventNames());
      });
    });

    // events@3, which sandboxed preloads used before, reports the function that
    // was registered when a once() wrapper is removed from a multi-listener
    // event; current Node.js reports the wrapper there, so this is checked
    // against the native implementation only.
    it("when a once() listener's removal is observed alongside other listeners", () => {
      const e = new (NativeEventEmitter as unknown as Impl)();
      const seen: any[][] = [];
      const onceHandler = () => seen.push(['once']);
      const other = () => seen.push(['other']);
      e.on('removeListener', (name: string, fn: Function) =>
        seen.push(['removed', name, fn === onceHandler, fn === other])
      );
      e.on('x', other);
      e.once('x', onceHandler);
      e.emit('x');
      e.emit('x');
      e.removeListener('x', other);
      expect(seen).to.deep.equal([
        ['other'],
        ['removed', 'x', true, false],
        ['once'],
        ['other'],
        ['removed', 'x', false, true]
      ]);
      expect(e.listenerCount('x')).to.equal(0);
    });

    it('when removing every listener of a large event', () => {
      sameAsNode((EventEmitter, log) => {
        const e = new EventEmitter();
        e.setMaxListeners(0);
        let removed = 0;
        e.on('removeListener', () => removed++);
        for (let i = 0; i < 10000; i++) e.on('big', () => {});
        e.removeAllListeners('big');
        log(removed, e.listenerCount('big'), e.eventNames());
      });
    });

    it('for subclasses and legacy constructor calls', () => {
      sameAsNode((EventEmitter, log) => {
        class Sub extends EventEmitter {
          value = 7;
          emit(name: string | symbol, ...args: any[]) {
            log('override', name);
            return super.emit(name, ...args);
          }
        }
        const s = new Sub();
        log(s instanceof EventEmitter, s.value, Object.getOwnPropertyNames(s));
        s.once('x', () => log('x'));
        s.emit('x');
        // once() removal goes through the public removeListener
        log(s.listenerCount('x'));
        function Legacy(this: any) {
          EventEmitter.call(this);
        }
        Object.setPrototypeOf(Legacy.prototype, EventEmitter.prototype);
        const l = new (Legacy as any)();
        l.on('y', () => log('legacy'));
        l.emit('y');
        log(Object.getOwnPropertyNames(l));
      });
    });

    it('for methods applied to plain objects', () => {
      sameAsNode((EventEmitter, log) => {
        const target: any = { name: 'plain' };
        EventEmitter.prototype.on.call(target, 'x', (v: any) => log('got', v));
        log(EventEmitter.prototype.emit.call(target, 'x', 1));
        log(EventEmitter.prototype.listenerCount.call(target, 'x'), Object.keys(target));
        log(EventEmitter.prototype.emit.call({}, 'nothing'));
        log(EventEmitter.prototype.listeners.call({}, 'nothing'));
      });
    });
  });

  describe('errors', () => {
    it('wraps a non-Error unhandled error event', () => {
      const e = new NativeEventEmitter();
      const fromString = captureThrow(() => e.emit('error', 'a string'));
      expect(fromString).to.be.an.instanceOf(Error);
      expect(fromString.message).to.equal('Unhandled error. (undefined)');
      expect(fromString.context).to.equal('a string');
      const fromObject = captureThrow(() => e.emit('error', { message: 'custom' }));
      expect(fromObject.message).to.equal('Unhandled error. (custom)');
      expect(fromObject.context).to.deep.equal({ message: 'custom' });
      const fromNothing = captureThrow(() => e.emit('error'));
      expect(fromNothing.message).to.equal('Unhandled error.');
      expect(fromNothing.context).to.equal(undefined);
    });

    it('validates listeners and max listener counts', () => {
      const e: any = new NativeEventEmitter();
      for (const call of [
        () => e.on('x', null),
        () => e.once('x', 'nope'),
        () => e.removeListener('x', {}),
        () => e.prependListener('x')
      ]) {
        const error = captureThrow(call);
        expect(error).to.be.an.instanceOf(TypeError);
        expect(error.message).to.match(
          /^The "listener" argument must be of type Function\. Received type (object|string|undefined)$/
        );
      }
      for (const value of [-1, '3', NaN]) {
        const error = captureThrow(() => e.setMaxListeners(value));
        expect(error).to.be.an.instanceOf(RangeError);
        expect(error.message).to.equal(
          `The value of "n" is out of range. It must be a non-negative number. Received ${value}.`
        );
      }
      expect(
        captureThrow(() => {
          NativeEventEmitter.defaultMaxListeners = -1;
        })
      ).to.be.an.instanceOf(RangeError);
      expect(NativeEventEmitter.defaultMaxListeners).to.equal(10);
    });

    it('throws when a method is called without a receiver', () => {
      const { on, emit } = new NativeEventEmitter();
      expect(() => (on as any)('x', () => {})).to.throw(TypeError);
      expect(() => (emit as any)('x')).to.throw(TypeError);
      expect((globalThis as any)._events).to.equal(undefined);
    });
  });

  describe('max listener warning', () => {
    it('is reported once per event name through console.warn', () => {
      const warnings: any[] = [];
      const originalWarn = console.warn;
      console.warn = (w: any) => warnings.push(w);
      try {
        const e = new NativeEventEmitter();
        e.setMaxListeners(1);
        const fn = () => {};
        e.on('leak', fn);
        e.on('leak', fn);
        e.on('leak', fn);
        // Churning listeners on an already-warned event does not warn again.
        e.removeListener('leak', fn);
        e.on('leak', fn);
      } finally {
        console.warn = originalWarn;
      }
      expect(warnings).to.have.lengthOf(1);
      expect(warnings[0]).to.be.an.instanceOf(Error);
      expect(warnings[0].name).to.equal('MaxListenersExceededWarning');
      expect(warnings[0].count).to.equal(2);
      expect(warnings[0].type).to.equal('leak');
      expect(warnings[0].message).to.equal(
        'Possible EventEmitter memory leak detected. 2 leak listeners added. Use emitter.setMaxListeners() to increase limit'
      );
    });
  });

  it("works with Node's events.once() helper", async () => {
    const e = new NativeEventEmitter();
    const p = once(e as any, 'x');
    e.emit('x', 1, 2);
    expect(await p).to.deep.equal([1, 2]);
    expect(e.listenerCount('x')).to.equal(0);
    expect(e.listenerCount('error')).to.equal(0);
  });

  describe('in a sandboxed preload', () => {
    afterEach(closeAllWindows);

    it('backs ipcRenderer and process', async () => {
      const w = new BrowserWindow({
        show: false,
        webPreferences: {
          sandbox: true,
          preload: path.join(__dirname, 'fixtures', 'module', 'preload-eventemitter.js')
        }
      });
      w.loadURL('about:blank');
      const [, rendererEventEmitterProperties] = await once(ipcMain, 'answer');
      expect(rendererEventEmitterProperties).to.deep.equal(
        Object.getOwnPropertyNames(NodeEventEmitter.prototype).sort()
      );
    });
  });
});
