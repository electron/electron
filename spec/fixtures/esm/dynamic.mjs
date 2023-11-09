const { app } = await import('electron');
const { exitWithApp } = await import('./exit.mjs');

exitWithApp(app);
