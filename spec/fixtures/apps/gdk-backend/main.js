const { app } = require('electron');

const { execFileSync } = require('node:child_process');

app.whenReady().then(() => {
  try {
    // A shell parameter expansion, not a JS template.
    // oxlint-disable-next-line no-template-curly-in-string
    const inherited = execFileSync('/bin/sh', ['-c', 'printf %s "${GDK_BACKEND-<unset>}"'], {
      timeout: 10_000
    }).toString();
    process.stdout.write(`GDK_BACKEND in child: ${inherited}\n`);
    app.quit();
  } catch (error) {
    console.error(error);
    app.exit(1);
  }
});
