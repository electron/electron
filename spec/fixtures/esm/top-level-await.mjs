import * as electron from 'electron';

// Cheeky delay
await new Promise((resolve) => setTimeout(resolve, 500));

console.log('Top level await, ready:', electron.app.isReady());
process.exit(0);
