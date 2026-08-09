// Reports the utility process's electron/js2c/* build-time code-cache
// status back to the parent for the js2c-code-cache spec.
const v8Util = process._linkedBinding('electron_common_v8_util');
process.parentPort.postMessage(v8Util.getJs2cCodeCacheStatus());
setTimeout(() => process.exit(0), 50);
