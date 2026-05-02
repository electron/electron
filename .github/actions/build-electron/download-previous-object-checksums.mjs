import { Octokit } from '@octokit/rest';

import { writeFileSync } from 'node:fs';

const token = process.env.GITHUB_TOKEN;
const repo = process.env.REPO;
const artifactName = process.env.ARTIFACT_NAME;
const branch = process.env.SEARCH_BRANCH;
const outputPath = process.env.OUTPUT_PATH;

const required = { GITHUB_TOKEN: token, REPO: repo, ARTIFACT_NAME: artifactName, SEARCH_BRANCH: branch, OUTPUT_PATH: outputPath };
const missing = Object.entries(required).filter(([, v]) => !v).map(([k]) => k);
if (missing.length > 0) {
  console.error(`Missing required environment variables: ${missing.join(', ')}`);
  process.exit(1);
}

const [owner, repoName] = repo.split('/');
const octokit = new Octokit({ auth: token });

async function main () {
  console.log(`Searching for artifact '${artifactName}' on branch '${branch}'...`);

  // Resolve the "Build" workflow name to an ID, mirroring how `gh run list --workflow` works
  // under the hood (it uses /repos/{owner}/{repo}/actions/workflows/{id}/runs).
  const { data: workflows } = await octokit.actions.listRepoWorkflows({ owner, repo: repoName });
  const buildWorkflow = workflows.workflows.find((w) => w.name === 'Build');
  if (!buildWorkflow) {
    console.log('Could not find "Build" workflow, continuing without previous checksums');
    return;
  }

  const { data: runs } = await octokit.actions.listWorkflowRuns({
    owner,
    repo: repoName,
    workflow_id: buildWorkflow.id,
    branch,
    status: 'completed',
    event: 'push',
    per_page: 20,
    exclude_pull_requests: true
  });

  for (const run of runs.workflow_runs) {
    const { data: artifacts } = await octokit.actions.listWorkflowRunArtifacts({
      owner,
      repo: repoName,
      run_id: run.id,
      name: artifactName
    });

    if (artifacts.artifacts.length > 0) {
      const artifact = artifacts.artifacts[0];
      console.log(`Found artifact in run ${run.id} (artifact ID: ${artifact.id}), downloading...`);

      // Non-archived artifacts are still downloaded from the /zip endpoint
      const response = await octokit.actions.downloadArtifact({
        owner,
        repo: repoName,
        artifact_id: artifact.id,
        archive_format: 'zip'
      });

      if (response.headers['content-type'] !== 'application/json') {
        console.error(`Unexpected content type for artifact download: ${response.headers['content-type']}`);
        console.error('Expected application/json, continuing without previous checksums');
        return;
      }

      writeFileSync(outputPath, JSON.stringify(response.data));
      console.log('Downloaded previous object checksums successfully');
      return;
    }
  }

  console.log(`No previous object checksums found in last ${runs.workflow_runs.length} runs, continuing without them`);
}

main().catch((err) => {
  console.error('Failed to download previous object checksums, continuing without them:', err.message);
  process.exit(0);
});
