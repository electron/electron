// Vitest externalises bare specifiers to native `import()`, but Electron only
// hooks CJS `require('electron')`. Alias 'electron' here so the runner ends up
// in CJS-land and gets the real module.
module.exports = require('electron');
