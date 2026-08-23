// Loading tls starts Node's off-main-thread CA certificate loader. Exiting
// straight away, before 'ready', used to race that thread against exit-time
// static destruction and abort().
require('node:tls');

const { app } = require('electron');

app.exit(123);
