// Captures original globals into a scope to ensure that userland modifications do
// not impact Electron.  Note that users doing:
//
// global.Promise.resolve = myFn
//
// Will mutate this captured one as well and that is OK.

export const Promise = global.Promise;
