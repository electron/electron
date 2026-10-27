const { contextBridge, ipcRenderer } = require('electron');

const evalTests = {
  evalConstructorName: () => globalThis.constructor.name
};

const tests = {
  testProcess: () => ({
    type: process.type,
    sandboxed: process.sandboxed,
    contextIsolated: process.contextIsolated,
    arch: process.arch,
    platform: process.platform,
    version: process.version,
    electronVersion: process.versions.electron,
    chromeVersion: process.versions.chrome,
    nodeVersion: process.versions.node,
    hasEnv: typeof process.env === 'object' && Object.keys(process.env).length > 0,
    hasExecPath: typeof process.execPath === 'string' && process.execPath.length > 0
  }),
  testGlobalScope: () => {
    const interfaces = [
      'AbortController',
      'AbortSignal',
      'CompressionStream',
      'CustomEvent',
      'DecompressionStream',
      'DOMException',
      'ErrorEvent',
      'Event',
      'EventTarget',
      'PromiseRejectionEvent',
      'TextDecoder',
      'TextDecoderStream',
      'TextEncoder',
      'TextEncoderStream',
      'URL',
      'URLPattern',
      'URLSearchParams',
      'atob',
      'btoa',
      'queueMicrotask',
      'reportError',
      'structuredClone'
    ];
    return {
      constructorName: globalThis.constructor.name,
      missing: interfaces.filter((name) => typeof globalThis[name] !== 'function')
    };
  },
  testReportError: async () => {
    reportError(new Error('reported from preload realm'));
    Promise.reject(new Error('rejected in preload realm'));
    // Unhandled rejections are reported after the microtask checkpoint.
    await new Promise((resolve) => queueMicrotask(resolve));
    return true;
  },
  testSend: (name, ...args) => {
    ipcRenderer.send(name, ...args);
  },
  testInvoke: async (name, ...args) => {
    const result = await ipcRenderer.invoke(name, ...args);
    return result;
  },
  testEvaluate: (testName, args) => {
    const func = evalTests[testName];
    const result = args ? contextBridge.executeInMainWorld({ func, args }) : contextBridge.executeInMainWorld({ func });
    return result;
  },
  testPrototypeLeak: () => {
    const checkPrototypes = (value) => {
      // Get prototype in preload world
      const prototype = Object.getPrototypeOf(value);
      const constructorName = prototype.constructor.name;

      const result = contextBridge.executeInMainWorld({
        func: (value) => {
          // Deeply check that value prototypes exist in the local world
          const check = (v) => {
            if (typeof v === 'undefined' || v === null) return true;
            const prototype = Object.getPrototypeOf(v);
            const constructorName = prototype.constructor.name;
            const localPrototype = globalThis[constructorName].prototype;
            if (prototype !== localPrototype) return false;
            if (Array.isArray(v)) return v.every(check);
            if (typeof v === 'object') return Object.values(v).every(check);
            if (typeof v === 'function') return check(v());
            return true;
          };
          return { protoMatches: check(value), value };
        },
        args: [value, constructorName]
      });

      // Deeply check that value prototypes exist in the local world
      const check = (v) => {
        if (typeof v === 'undefined' || v === null) return true;
        const prototype = Object.getPrototypeOf(v);
        const constructorName = prototype.constructor.name;
        const localPrototype = globalThis[constructorName].prototype;
        if (prototype !== localPrototype) return false;
        if (Array.isArray(v)) return v.every(check);
        if (typeof v === 'object') return Object.values(v).every(check);
        if (typeof v === 'function') return check(v());
        return true;
      };

      return (
        // Prototype matched in main world
        result.protoMatches &&
        // Returned value matches prototype
        check(result.value)
      );
    };

    const values = [
      123,
      'string',
      true,
      [],
      [123, 'string', true, ['foo']],
      Symbol('foo'),
      10n,
      {},
      Promise.resolve(),
      () => {},
      () => () => null,
      { [Symbol('foo')]: 123 }
    ];

    for (const value of values) {
      if (!checkPrototypes(value)) {
        const constructorName = Object.getPrototypeOf(value).constructor.name;
        return `${constructorName} (${value}) leaked in service worker preload`;
      }
    }

    return true;
  }
};

ipcRenderer.on('test', async (_event, uuid, name, ...args) => {
  console.debug(`running test ${name} for ${uuid}`);
  try {
    const result = await tests[name]?.(...args);
    console.debug(`responding test ${name} for ${uuid}`);
    ipcRenderer.send(`test-result-${uuid}`, { error: false, result });
  } catch (error) {
    console.debug(`erroring test ${name} for ${uuid}`);
    ipcRenderer.send(`test-result-${uuid}`, {
      error: true,
      result: error.message
    });
  }
});
