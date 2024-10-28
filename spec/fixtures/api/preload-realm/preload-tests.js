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
    ipcRenderer.send(`test-result-${uuid}`, { error: true, result: error.message });
  }
});
