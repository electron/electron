var atom = require('atom');

atom.browserMainParts.preMainMessageLoopRun = function() {
  console.log('Create new window');
}
