const { app } = require('electron');

process.stdout.write(app.commandLine.getSwitchValue('ozone-platform'));
process.exit(0);
