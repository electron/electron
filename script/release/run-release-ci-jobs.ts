import { Octokit } from '@octokit/rest';
import got, { OptionsOfTextResponseBody } from 'got';

import * as assert from 'node:assert';

import { createGitHubTokenStrategy } from './github-token';
import { ELECTRON_ORG, ELECTRON_REPO } from './types';

const octokit = new Octokit({
  authStrategy: createGitHubTokenStrategy(ELECTRON_REPO)
});

const BUILD_APPVEYOR_URL = 'https://ci.appveyor.com/api/builds';
const GH_ACTIONS_PIPELINE_URL = 'https://github.com/electron/electron/actions';

const GH_ACTIONS_WAIT_TIME = process.env.GH_ACTIONS_WAIT_TIME ? parseInt(process.env.GH_ACTIONS_WAIT_TIME, 10) : 30000;

const appVeyorJobs = {
  'electron-x64': 'electron-x64-release',
  'electron-ia32': 'electron-ia32-release',
  'electron-woa': 'electron-woa-release'
};

const ghActionsPublishWorkflows = [
  'linux-publish',
  'macos-publish'
] as const;

let jobRequestedCount = 0;

type ReleaseBuildRequestOptions = {
  auth?: {
    bearer?: string;
  };
  url: string;
  headers: Record<string, string>;
  body: string,
  method: 'GET' | 'POST';
}

async function makeRequest ({ auth, url, headers, body, method }: ReleaseBuildRequestOptions) {
  const clonedHeaders = {
    ...(headers || {})
  };
  if (auth?.bearer) {
    clonedHeaders.Authorization = `Bearer ${auth.bearer}`;
  }

  const options: OptionsOfTextResponseBody = {
    headers: clonedHeaders,
    body,
    method
  };

  const response = await got(url, options);

  if (response.statusCode < 200 || response.statusCode >= 300) {
    console.error('Error: ', `(status ${response.statusCode})`, response.body);
    throw new Error(`Unexpected status code ${response.statusCode} from ${url}`);
  }
  return JSON.parse(response.body);
}

type GitHubActionsCallOptions = {
  ghRelease?: boolean;
  newVersion: string;
  runningPublishWorkflows?: boolean;
}

async function githubActionsCall (targetBranch: string, workflowName: string, options: GitHubActionsCallOptions) {
  console.log(`Triggering GitHub Actions to run build job: ${workflowName} on branch: ${targetBranch} with release flag.`);
  const buildRequest = {
    branch: targetBranch,
    parameters: {} as Record<string, string | boolean>
  };
  if (options.ghRelease) {
    buildRequest.parameters['upload-to-storage'] = '0';
  } else {
    buildRequest.parameters['upload-to-storage'] = '1';
  }
  buildRequest.parameters[`run-${workflowName}`] = true;

  jobRequestedCount++;
  try {
    const commits = await octokit.repos.listCommits({
      owner: ELECTRON_ORG,
      repo: ELECTRON_REPO,
      sha: targetBranch,
      per_page: 5
    });
    if (!commits.data.length) {
      console.error('Could not fetch most recent commits for GitHub Actions, returning early');
    }

    await octokit.actions.createWorkflowDispatch({
      repo: ELECTRON_REPO,
      owner: ELECTRON_ORG,
      workflow_id: `${workflowName}.yml`,
      ref: `refs/tags/${options.newVersion}`,
      inputs: {
        ...buildRequest.parameters
      }
    });

    const runNumber = await getGitHubActionsRun(workflowName, commits.data[0].sha);
    if (runNumber === -1) {
      return;
    }

    console.log(`GitHub Actions release build pipeline ${runNumber} for ${workflowName} triggered.`);
    const runUrl = `${GH_ACTIONS_PIPELINE_URL}/runs/${runNumber}`;

    if (options.runningPublishWorkflows) {
      console.log(`GitHub Actions release workflow request for ${workflowName} successful.  Check ${runUrl} for status.`);
    } else {
      console.log(`GitHub Actions release build workflow running at ${GH_ACTIONS_PIPELINE_URL}/runs/${runNumber} for ${workflowName}.`);
      console.log(`GitHub Actions release build request for ${workflowName} successful.  Check ${runUrl} for status.`);
    }
  } catch (err) {
    console.log('Error calling GitHub Actions: ', err);
  }
}

async function getGitHubActionsRun (workflowName: string, headCommit: string) {
  let runNumber = 0;
  let actionRun;
  while (runNumber === 0) {
    const actionsRuns = await octokit.actions.listWorkflowRuns({
      repo: ELECTRON_REPO,
      owner: ELECTRON_ORG,
      workflow_id: `${workflowName}.yml`
    });

    if (!actionsRuns.data.workflow_runs.length) {
      console.log(`No current workflow_runs found for ${workflowName}, response was: ${actionsRuns.data.workflow_runs}`);
      runNumber = -1;
      break;
    }

    for (const run of actionsRuns.data.workflow_runs) {
      if (run.head_sha === headCommit) {
        console.log(`GitHub Actions run ${run.html_url} found for ${headCommit}, waiting on status.`);
        actionRun = run;
        break;
      }
    }

    if (actionRun) {
      switch (actionRun.status) {
        case 'in_progress':
        case 'pending':
        case 'queued':
        case 'requested':
        case 'waiting': {
          if (actionRun.id && !isNaN(actionRun.id)) {
            console.log(`GitHub Actions run ${actionRun.status} for ${actionRun.html_url}.`);
            runNumber = actionRun.id;
          }
          break;
        }
        case 'action_required':
        case 'cancelled':
        case 'failure':
        case 'skipped':
        case 'timed_out':
        case 'failed': {
          console.log(`Error workflow run returned a status of ${actionRun.status} for ${actionRun.html_url}`);
          runNumber = -1;
          break;
        }
      }
      await new Promise(resolve => setTimeout(resolve, GH_ACTIONS_WAIT_TIME));
    }
  }
  return runNumber;
}

type AppVeyorCallOptions = {
  ghRelease?: boolean;
  commit?: string;
}

async function callAppVeyor (targetBranch: string, job: keyof typeof appVeyorJobs, options: AppVeyorCallOptions) {
  console.log(`Triggering AppVeyor to run build job: ${job} on branch: ${targetBranch} with release flag.`);
  const environmentVariables: Record<string, string | number> = {
    ELECTRON_RELEASE: 1,
    APPVEYOR_BUILD_WORKER_CLOUD: 'electronhq-16-core'
  };

  if (!options.ghRelease) {
    environmentVariables.UPLOAD_TO_STORAGE = 1;
  }

  const requestOpts = {
    url: BUILD_APPVEYOR_URL,
    auth: {
      bearer: process.env.APPVEYOR_CLOUD_TOKEN
    },
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      accountName: 'electron-bot',
      projectSlug: appVeyorJobs[job],
      branch: targetBranch,
      commitId: options.commit || undefined,
      environmentVariables
    }),
    method: 'POST'
  } as const;
  jobRequestedCount++;

  try {
    const { version } = await makeRequest(requestOpts);
    const buildUrl = `https://ci.appveyor.com/project/electron-bot/${appVeyorJobs[job]}/build/${version}`;
    console.log(`AppVeyor release build request for ${job} successful.  Check build status at ${buildUrl}`);
  } catch (err: any) {
    if (err.response?.body) {
      console.error('Could not call AppVeyor: ', {
        statusCode: err.response.statusCode,
        body: JSON.parse(err.response.body)
      });
    } else {
      console.error('Error calling AppVeyor:', err);
    }
  }
}

type BuildAppVeyorOptions = {
  job?: keyof typeof appVeyorJobs;
} & AppVeyorCallOptions;

async function buildAppVeyor (targetBranch: string, options: BuildAppVeyorOptions) {
  const validJobs = Object.keys(appVeyorJobs) as (keyof typeof appVeyorJobs)[];
  if (options.job) {
    assert(validJobs.includes(options.job), `Unknown AppVeyor CI job name: ${options.job}.  Valid values are: ${validJobs}.`);
    await callAppVeyor(targetBranch, options.job, options);
  } else {
    for (const job of validJobs) {
      await callAppVeyor(targetBranch, job, options);
    }
  }
}

type BuildGHActionsOptions = {
  job?: typeof ghActionsPublishWorkflows[number];
  arch?: string;
} & GitHubActionsCallOptions;

async function buildGHActions (targetBranch: string, options: BuildGHActionsOptions) {
  if (options.job) {
    assert(ghActionsPublishWorkflows.includes(options.job), `Unknown GitHub Actions workflow name: ${options.job}. Valid values are: ${ghActionsPublishWorkflows}.`);
    await githubActionsCall(targetBranch, options.job, options);
  } else {
    assert(!options.arch, 'Cannot provide a single architecture while building all workflows, please specify a single workflow via --workflow');
    options.runningPublishWorkflows = true;
    for (const job of ghActionsPublishWorkflows) {
      await githubActionsCall(targetBranch, job, options);
    }
  }
}

type RunReleaseOptions = ({
  ci: 'GitHubActions'
} & BuildGHActionsOptions) | ({
  ci: 'AppVeyor'
} & BuildAppVeyorOptions) | ({
  ci: undefined,
} & BuildAppVeyorOptions & BuildGHActionsOptions);

export async function runReleaseCIJobs (targetBranch: string, options: RunReleaseOptions) {
  if (options.ci) {
    switch (options.ci) {
      case 'GitHubActions': {
        await buildGHActions(targetBranch, options);
        break;
      }
      case 'AppVeyor': {
        await buildAppVeyor(targetBranch, options);
        break;
      }
      default: {
        console.log(`Error! Unknown CI: ${(options as any).ci}.`);
        process.exit(1);
      }
    }
  } else {
    await Promise.all([
      buildAppVeyor(targetBranch, options),
      buildGHActions(targetBranch, options)
    ]);
  }
  console.log(`${jobRequestedCount} jobs were requested.`);
}
