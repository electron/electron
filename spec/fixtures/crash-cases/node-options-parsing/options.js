const path = require('node:path');

if (
  process.env.NODE_OPTIONS &&
  process.env.NODE_OPTIONS.trim() ===
    `--trace-warnings --require ${path.join(__dirname, 'preload.js')} --trace-warnings`
) {
  process.exit(0);
} else {
  process.exit(1);
}
