#!/usr/bin/env node

import { Octokit } from '@octokit/rest';
import * as chalk from 'chalk';

import { createGitHubTokenStrategy } from './github-token';
import { ELECTRON_ORG, ELECTRON_REPO, ElectronReleaseRepo, NIGHTLY_REPO } from './types';

const pass = chalk.green('✓');
const fail = chalk.red('✗');

async function deleteDraft (releaseId: string, targetRepo: ElectronReleaseRepo) {
  const octokit = new Octokit({
    authStrategy: createGitHubTokenStrategy(targetRepo)
  });

  try {
    const result = await octokit.repos.getRelease({
      owner: ELECTRON_ORG,
      repo: targetRepo,
      release_id: parseInt(releaseId, 10)
    });
    if (!result.data.draft) {
      console.log(`${fail} published releases cannot be deleted.`);
      return false;
    } else {
      await octokit.repos.deleteRelease({
        owner: ELECTRON_ORG,
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

async function deleteTag (tag: string, targetRepo: ElectronReleaseRepo) {
  const octokit = new Octokit({
    authStrategy: createGitHubTokenStrategy(targetRepo)
  });

  try {
    await octokit.git.deleteRef({
      owner: ELECTRON_ORG,
      repo: targetRepo,
      ref: `tags/${tag}`
    });
    console.log(`${pass} successfully deleted tag ${tag} from ${targetRepo}`);
  } catch (err) {
    console.log(`${fail} couldn't delete tag ${tag} from ${targetRepo}: `, err);
  }
}

type CleanOptions = {
  releaseID?: string;
  tag: string;
}

export async function cleanReleaseArtifacts ({ releaseID, tag }: CleanOptions) {
  const releaseId = releaseID && releaseID.length > 0 ? releaseID : null;
  const isNightly = tag.includes('nightly');

  if (releaseId) {
    if (isNightly) {
      await deleteDraft(releaseId, NIGHTLY_REPO);

      // We only need to delete the Electron tag since the
      // nightly tag is only created at publish-time.
      await deleteTag(tag, ELECTRON_REPO);
    } else {
      const deletedElectronDraft = await deleteDraft(releaseId, ELECTRON_REPO);
      // don't delete tag unless draft deleted successfully
      if (deletedElectronDraft) {
        await deleteTag(tag, ELECTRON_REPO);
      }
    }
  } else {
    await Promise.all([
      deleteTag(tag, ELECTRON_REPO),
      deleteTag(tag, NIGHTLY_REPO)
    ]);
  }

  console.log(`${pass} failed release artifact cleanup complete`);
}
