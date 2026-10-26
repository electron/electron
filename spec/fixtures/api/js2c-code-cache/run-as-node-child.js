// Reports an ELECTRON_RUN_AS_NODE child's builtin code-cache status back to
// the parent (over the child_process.fork IPC channel) for the
// js2c-code-cache spec.
const v8Util = process._linkedBinding('electron_common_v8_util');
process.send(v8Util.getJs2cCodeCacheStatus(), () => process.exit(0));
