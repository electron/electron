// Drives globalShortcut against the GlobalShortcuts desktop portal and prints
// the outcome as one JSON line. Run by api-global-shortcut-spec.ts with
// --ozone-platform=headless --enable-features=GlobalShortcutsPortal against
// the mock portal on the spec runner's fake session bus.
const { app, globalShortcut } = require('electron');

const { setTimeout } = require('node:timers/promises');

const scenario = process.argv.find((arg) => arg.startsWith('--scenario='))?.slice('--scenario='.length);

const list = async () => {
  try {
    return { shortcuts: await globalShortcut.listShortcuts() };
  } catch (error) {
    return { error: error.message };
  }
};

app.whenReady().then(async () => {
  const result = { scenario };
  if (scenario === 'list') {
    result.first = await list();
    result.concurrent = await Promise.all([list(), list(), list()]);
  } else if (scenario === 'register') {
    // --register=<accelerator> entries are registered in order, then listed.
    result.registered = {};
    for (const arg of process.argv) {
      if (!arg.startsWith('--register=')) continue;
      const accelerator = arg.slice('--register='.length);
      result.registered[accelerator] = globalShortcut.register(accelerator, () => {});
      // Let the portal round trips for this registration settle.
      await setTimeout(1000);
    }
    result.after = await list();
    globalShortcut.unregisterAll();
  }
  process.stdout.write(JSON.stringify(result) + '\n');
  app.quit();
});
