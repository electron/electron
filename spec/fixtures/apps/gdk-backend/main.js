const { app } = require('electron');

const { execFileSync } = require('node:child_process');

app.whenReady().then(() => {
  const inherited = execFileSync(
    process.execPath,
    ['-e', "process.stdout.write(process.env.GDK_BACKEND ?? '<unset>')"],
    {
      env: { ...process.env, ELECTRON_RUN_AS_NODE: '1' }
    }
  ).toString();
  process.stdout.write(`GDK_BACKEND in child: ${inherited}\n`);
  app.quit();
});
