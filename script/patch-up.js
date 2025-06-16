const { appCredentialsFromString, getAuthOptionsForRepo } = require('@electron/github-app-auth');

const { Octokit } = require('@octokit/rest');

const cp = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');

const { PATCH_UP_APP_CREDS } = process.env;

const REPO_OWNER = 'electron';
const REPO_NAME = 'electron';

async function getAllPatchFiles (dir) {
  const files = [];

  async function walkDir (currentDir) {
    const entries = await fs.promises.readdir(currentDir, { withFileTypes: true });

    for (const entry of entries) {
      const fullPath = path.join(currentDir, entry.name);

      if (entry.isDirectory()) {
        await walkDir(fullPath);
      } else if (entry.isFile() && (entry.name.endsWith('.patch') || entry.name === 'README.md' || entry.name === 'config.json')) {
        const relativePath = path.relative(process.cwd(), fullPath);
        const content = await fs.promises.readFile(fullPath, 'utf8');
        files.push({
          path: relativePath,
          content
        });
      }
    }
  }

  await walkDir(dir);
  return files;
}

function getCurrentCommitSha () {
  const result = cp.spawnSync('git', ['rev-parse', 'HEAD'], { encoding: 'utf8' });
  if (result.status !== 0) {
    throw new Error('Failed to get current commit SHA');
  }
  return result.stdout.trim();
}

function getCurrentBranch () {
  const result = cp.spawnSync('git', ['symbolic-ref', '--short', 'HEAD'], { encoding: 'utf8' });
  if (result.status !== 0) {
    throw new Error('Failed to get current branch');
  }
  return result.stdout.trim();
}

async function main () {
  if (!PATCH_UP_APP_CREDS) {
    throw new Error('PATCH_UP_APP_CREDS environment variable not set');
  }

  const octokit = new Octokit({
    ...await getAuthOptionsForRepo({
      name: REPO_OWNER,
      owner: REPO_NAME
    }, appCredentialsFromString(PATCH_UP_APP_CREDS))
  });

  const patchesDir = path.join(process.cwd(), 'patches');

  // Get current git state
  const currentCommitSha = getCurrentCommitSha();
  const currentBranch = getCurrentBranch();

  // Get the tree SHA from the current commit
  const currentCommit = await octokit.git.getCommit({
    owner: REPO_OWNER,
    repo: REPO_NAME,
    commit_sha: currentCommitSha
  });
  const baseTreeSha = currentCommit.data.tree.sha;

  // Find all patch files
  const patchFiles = await getAllPatchFiles(patchesDir);

  if (patchFiles.length === 0) {
    throw new Error('No patch files found');
  }

  console.log(`Found ${patchFiles.length} patch files`);

  // Create a new tree with the patch files
  const tree = patchFiles.map(file => ({
    path: file.path,
    mode: '100644',
    type: 'blob',
    content: file.content
  }));

  const treeResponse = await octokit.git.createTree({
    owner: REPO_OWNER,
    repo: REPO_NAME,
    base_tree: baseTreeSha,
    tree
  });

  // Create a new commit
  const commitMessage = 'chore: update patches';
  const commitResponse = await octokit.git.createCommit({
    owner: REPO_OWNER,
    repo: REPO_NAME,
    message: commitMessage,
    tree: treeResponse.data.sha,
    parents: [currentCommitSha]
  });

  // Update the branch reference
  await octokit.git.updateRef({
    owner: REPO_OWNER,
    repo: REPO_NAME,
    ref: `heads/${currentBranch}`,
    sha: commitResponse.data.sha
  });

  console.log(`Successfully pushed commit ${commitResponse.data.sha} to ${currentBranch}`);
}

if (require.main === module) {
  main().catch((err) => {
    console.error(err);
    process.exit(1);
  });
}
