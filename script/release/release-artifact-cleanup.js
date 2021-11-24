#!/usr/bin/env node

if (!process.env.CI) require('dotenv-safe').load();
const args = require('minimist')(process.argv.slice(2), {
  string: ['tag', 'releaseID'],
  default: { releaseID: '' }
});
const { execSync } = require('child_process');
const { GitProcess } = require('dugite');
const { getCurrentBranch, ELECTRON_DIR } = require('../lib/utils.js');
const { Octokit } = require('@octokit/rest');

const octokit = new Octokit({
  auth: process.env.ELECTRON_GITHUB_TOKEN
});

require('colors');
const pass = '✓'.green;
const fail = '✗'.red;

function getLastBumpCommit (tag) {
  const data = execSync(`git log -n1 --grep "Bump ${tag}" --format='format:{"hash": "%H", "message": "%s"}'`).toString();
  return JSON.parse(data);
}

async function revertBumpCommit (tag) {
  const branch = await getCurrentBranch();
  const commitToRevert = getLastBumpCommit(tag).hash;
  await GitProcess.exec(['pull', '--rebase']);
  await GitProcess.exec(['revert', commitToRevert], ELECTRON_DIR);
  const pushDetails = await GitProcess.exec(['push', 'origin', `HEAD:${branch}`, '--follow-tags'], ELECTRON_DIR);
  if (pushDetails.exitCode === 0) {
    console.log(`${pass} successfully reverted release commit.`);
  } else {
    const error = GitProcess.parseError(pushDetails.stderr);
    console.error(`${fail} could not push release commit: `, error);
    process.exit(1);
  }
}

async function deleteDraft (releaseId, targetRepo) {
  try {
    const result = await octokit.repos.getRelease({
      owner: 'electron',
      repo: targetRepo,
      release_id: parseInt(releaseId, 10)
    });
    if (!result.data.draft) {
      console.log(`${fail} published releases cannot be deleted.`);
      return false;
    } else {
      await octokit.repos.deleteRelease({
        owner: 'electron',
        repo: targetRepo,
        release_id: result.data.id
      });
    }
    console.log(`${pass} successfully deleted draft with id ${releaseId} from ${targetRepo}`);
    return true;
  } catch (err) {
    console.error(`${fail} couldn't delete draft with id ${releaseId} from ${targetRepo}: `, err);
    return false;
  }
}

async function deleteTag (tag, targetRepo) {
  try {
    await octokit.git.deleteRef({
      owner: 'electron',
      repo: targetRepo,
      ref: `tags/${tag}`
    });
    console.log(`${pass} successfully deleted tag ${tag} from ${targetRepo}`);
  } catch (err) {
    console.log(`${fail} couldn't delete tag ${tag} from ${targetRepo}: `, err);
  }
}

async function cleanReleaseArtifacts () {
  const releaseId = args.releaseID.length > 0 ? args.releaseID : null;
  const isNightly = args.tag.includes('nightly');

  // try to revert commit regardless of tag and draft deletion status
  await revertBumpCommit(args.tag);

  if (releaseId) {
    if (isNightly) {
      await deleteDraft(releaseId, 'nightlies');

      // We only need to delete the Electron tag since the
      // nightly tag is only created at publish-time.
      await deleteTag(args.tag, 'electron');
    } else {
      const deletedElectronDraft = await deleteDraft(releaseId, 'electron');
      // don't delete tag unless draft deleted successfully
      if (deletedElectronDraft) {
        await deleteTag(args.tag, 'electron');
      }
    }
  } else {
    await Promise.all([
      deleteTag(args.tag, 'electron'),
      deleteTag(args.tag, 'nightlies')
    ]);
  }

  console.log(`${pass} failed release artifact cleanup complete`);
}

cleanReleaseArtifacts();
