const { appCredentialsFromString, getTokenForRepo } = require('@electron/github-app-auth');

const cp = require('node:child_process');

const { PATCH_UP_APP_CREDS } = process.env;

async function main () {
  if (!PATCH_UP_APP_CREDS) {
    throw new Error('PATCH_UP_APP_CREDS environment variable not set');
  }

  const token = await getTokenForRepo(
    {
      name: 'electron',
      owner: 'electron'
    },
    appCredentialsFromString(PATCH_UP_APP_CREDS)
  );

  const remoteURL = `https://x-access-token:${token}@github.com/electron/electron.git`;

  // NEVER LOG THE OUTPUT OF THIS COMMAND
  // GIT LEAKS THE ACCESS CREDENTIALS IN CONSOLE LOGS
  const { status } = cp.spawnSync('git', ['push', '--set-upstream', remoteURL], {
    stdio: 'ignore'
  });

  if (status !== 0) {
    throw new Error('Failed to push to target branch');
  }
}

if (require.main === module) {
  main().catch((err) => {
    console.error(err);
    process.exit(1);
  });
}
