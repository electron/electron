import * as electron from 'electron';

try {
  electron.app.disableHardwareAcceleration();
} catch {
  process.exit(1);
}

process.exit(0);
