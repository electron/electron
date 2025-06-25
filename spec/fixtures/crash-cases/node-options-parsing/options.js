const path = require('node:path');

if (process.env.NODE_OPTIONS &&
    process.env.NODE_OPTIONS.trim() === `--allow-addons --require ${path.join(__dirname, 'preload.js')} --allow-addons`) {
  process.exit(0);
} else {
  process.exit(1);
}
