const { app, Menu } = require('electron');

function output (value) {
  process.stdout.write(JSON.stringify(value));
  process.stdout.end();

  app.quit();
}

try {
  let expectedMenu;

  if (app.commandLine.hasSwitch('custom-menu')) {
    expectedMenu = new Menu();
    Menu.setApplicationMenu(expectedMenu);
  } else if (app.commandLine.hasSwitch('null-menu')) {
    expectedMenu = null;
    Menu.setApplicationMenu(null);
  }

  app.whenReady().then(() => {
    setImmediate(() => {
      try {
        output(Menu.getApplicationMenu() === expectedMenu);
      } catch {
        output(null);
      }
    });
  });
} catch {
  output(null);
}
