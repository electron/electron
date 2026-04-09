const { contextBridge, ipcRenderer } = require('electron');

const evalTests = {
  evalConstructorName: () => globalThis.constructor.name
};

const tests = {
  testSend: (name, ...args) => {
    ipcRenderer.send(name, ...args);
  },
  testInvoke: async (name, ...args) => {
    const result = await ipcRenderer.invoke(name, ...args);
    return result;
  },
  testEvaluate: (testName, args) => {
    const func = evalTests[testName];
    const result = args
      ? contextBridge.executeInMainWorld({ func, args })
      : contextBridge.executeInMainWorld({ func });
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
