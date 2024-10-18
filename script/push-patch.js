const { appCredentialsFromString, getTokenForRepo } = require('@electron/github-app-auth');

const cp = require('node:child_process');

async function main () {
  const token = await getTokenForRepo(
    {
      name: 'electron',
      owner: 'electron'
    },
    appCredentialsFromString(process.env.PATCH_UP_APP_CREDS)
  );
  const remoteURL = `https://x-access-token:${token}@github.com/electron/electron.git`;
  // NEVER LOG THE OUTPUT OF THIS COMMAND
  // GIT LEAKS THE ACCESS CREDENTIALS IN CONSOLE LOGS
  const { status } = cp.spawnSync('git', ['push', '--set-upstream', remoteURL], {
    stdio: 'ignore'
  });
  if (status !== 0) {
    console.error('Failed to push to target branch');
    process.exit(1);
  }
}

if (require.main === module) {
  main().catch((err) => {
    console.error(err);
    process.exit(1);
  });
}
