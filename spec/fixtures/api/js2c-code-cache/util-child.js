// Reports the utility process's electron/js2c/* build-time code-cache
// status back to the parent for the js2c-code-cache spec.
const v8Util = process._linkedBinding('electron_common_v8_util');
// Exits once the parent has acknowledged the status: process.exit() does not
// wait for the asynchronous postMessage() write to reach the parent.
process.parentPort.once('message', () => process.exit(0));
process.parentPort.postMessage(v8Util.getJs2cCodeCacheStatus());
