const {
  registerEmbedderCallbacks,
  embedderCallbackCounts,
  runImportMetaCallback,
  queueUvWork,
  Wrapped,
  collectGarbageWithoutContext
} = require('@electron-ci/object-wrap');
const v8 = require('node:v8');

module.exports = async ({ useCounter = true } = {}) => {
  v8.setFlagsFromString('--expose-gc');
  registerEmbedderCallbacks(useCounter);
  Promise.reject(new Error('expected rejection')).catch(() => {});
  if (useCounter) {
    Date.parse('2000 01 01');
  }
  await import('cfi-addon-boundary-missing').catch(() => {});
  if (!runImportMetaCallback()) {
    throw new Error('Failed to evaluate import.meta module');
  }
  queueUvWork();
  queueMicrotask(() => {});
  for (let attempts = 0; attempts < 100; attempts++) {
    await new Promise((resolve) => setImmediate(resolve));
    if (embedderCallbackCounts()[5] > 0) {
      break;
    }
  }

  const [rejections, microtasks, interrupts, useCounters, importMeta, uvWork, dynamicImports] =
    embedderCallbackCounts();
  if (
    rejections < 1 ||
    microtasks < 1 ||
    interrupts < 1 ||
    (useCounter && useCounters < 1) ||
    importMeta < 1 ||
    uvWork < 1 ||
    dynamicImports < 1
  ) {
    throw new Error(
      `Missing V8 callbacks: ${rejections} rejections, ${microtasks} microtasks, ` +
        `${interrupts} interrupts, ${useCounters} use counters, ${importMeta} import-meta, ` +
        `${uvWork} libuv work, ${dynamicImports} dynamic imports`
    );
  }

  Array.from({ length: 100 }, () => new Wrapped());
  collectGarbageWithoutContext();
  globalThis.cfiAddonBoundaryKeepAlive = new Wrapped();

  return { rejections, microtasks, interrupts, useCounters, importMeta, uvWork, dynamicImports };
};
