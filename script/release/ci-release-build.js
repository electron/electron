if (!process.env.CI) require('dotenv-safe').load();

const assert = require('node:assert');
const got = require('got');

const { Octokit } = require('@octokit/rest');
const octokit = new Octokit({
  auth: process.env.ELECTRON_GITHUB_TOKEN
});

const BUILD_APPVEYOR_URL = 'https://ci.appveyor.com/api/builds';
const CIRCLECI_PIPELINE_URL = 'https://circleci.com/api/v2/project/gh/electron/electron/pipeline';
const GH_ACTIONS_PIPELINE_URL = 'https://github.com/electron/electron/actions';
const GH_ACTIONS_API_URL = '/repos/electron/electron/actions';

const CIRCLECI_WAIT_TIME = process.env.CIRCLECI_WAIT_TIME || 30000;
const GH_ACTIONS_WAIT_TIME = process.env.GH_ACTIONS_WAIT_TIME || 30000;

const appVeyorJobs = {
  'electron-x64': 'electron-x64-release',
  'electron-ia32': 'electron-ia32-release',
  'electron-woa': 'electron-woa-release'
};

const circleCIPublishWorkflows = [
  'linux-publish',
  'macos-publish'
];

const circleCIPublishIndividualArches = {
  'macos-publish': ['osx-x64', 'mas-x64', 'osx-arm64', 'mas-arm64'],
  'linux-publish': ['arm', 'arm64', 'x64']
};

const ghActionsPublishWorkflows = [
  'macos-publish'
];

const ghActionsPublishIndividualArches = {
  'macos-publish': ['osx-x64', 'mas-x64', 'osx-arm64', 'mas-arm64']
};

let jobRequestedCount = 0;

async function makeRequest ({ auth, username, password, url, headers, body, method }) {
  const clonedHeaders = {
    ...(headers || {})
  };
  if (auth?.bearer) {
    clonedHeaders.Authorization = `Bearer ${auth.bearer}`;
  }

  const options = {
    headers: clonedHeaders,
    body,
    method
  };

  if (username || password) {
    options.username = username;
    options.password = password;
  }

  const response = await got(url, options);

  if (response.statusCode < 200 || response.statusCode >= 300) {
    console.error('Error: ', `(status ${response.statusCode})`, response.body);
    throw new Error(`Unexpected status code ${response.statusCode} from ${url}`);
  }
  return JSON.parse(response.body);
}

async function githubActionsCall (targetBranch, workflowName, options) {
  console.log(`Triggering GitHub Actions to run build job: ${workflowName} on branch: ${targetBranch} with release flag.`);
  const buildRequest = {
    branch: targetBranch,
    parameters: {}
  };
  if (options.ghRelease) {
    buildRequest.parameters['upload-to-storage'] = '0';
  } else {
    buildRequest.parameters['upload-to-storage'] = '1';
  }
  buildRequest.parameters[`run-${workflowName}`] = true;
  if (options.arch) {
    const validArches = ghActionsPublishIndividualArches[workflowName];
    assert(validArches.includes(options.arch), `Unknown GitHub Actions architecture "${options.arch}".  Valid values are ${JSON.stringify(validArches)}`);
    buildRequest.parameters['macos-publish-arch-limit'] = options.arch;
  }

  jobRequestedCount++;
  try {
    const commits = await octokit.repos.listCommits({
      owner: 'electron',
      repo: 'electron',
      sha: targetBranch,
      per_page: 5
    });
    if (!commits.data.length) {
      console.error('Could not fetch most recent commits for GitHub Actions, returning early');
    }

    await octokit.request(`POST ${GH_ACTIONS_API_URL}/workflows/${workflowName}.yml/dispatches`, {
      ref: buildRequest.branch,
      inputs: {
        ...buildRequest.parameters
      },
      headers: {
        'X-GitHub-Api-Version': '2022-11-28'
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

async function circleCIcall (targetBranch, workflowName, options) {
  console.log(`Triggering CircleCI to run build job: ${workflowName} on branch: ${targetBranch} with release flag.`);
  const buildRequest = {
    branch: targetBranch,
    parameters: {}
  };
  if (options.ghRelease) {
    buildRequest.parameters['upload-to-storage'] = '0';
  } else {
    buildRequest.parameters['upload-to-storage'] = '1';
  }
  buildRequest.parameters[`run-${workflowName}`] = true;
  if (options.arch) {
    const validArches = circleCIPublishIndividualArches[workflowName];
    assert(validArches.includes(options.arch), `Unknown CircleCI architecture "${options.arch}".  Valid values are ${JSON.stringify(validArches)}`);
    buildRequest.parameters['macos-publish-arch-limit'] = options.arch;
  }

  jobRequestedCount++;
  // The logic below expects that the CircleCI workflows for releases each
  // contain only one job in order to maintain compatibility with sudowoodo.
  // If the workflows are changed in the CircleCI config.yml, this logic will
  // also need to be changed as well as possibly changing sudowoodo.
  try {
    const circleResponse = await circleCIRequest(CIRCLECI_PIPELINE_URL, 'POST', buildRequest);
    console.log(`CircleCI release build pipeline ${circleResponse.id} for ${workflowName} triggered.`);
    const workflowId = await getCircleCIWorkflowId(circleResponse.id);
    if (workflowId === -1) {
      return;
    }
    const workFlowUrl = `https://circleci.com/workflow-run/${workflowId}`;
    if (options.runningPublishWorkflows) {
      console.log(`CircleCI release workflow request for ${workflowName} successful.  Check ${workFlowUrl} for status.`);
    } else {
      console.log(`CircleCI release build workflow running at https://circleci.com/workflow-run/${workflowId} for ${workflowName}.`);
      const jobNumber = await getCircleCIJobNumber(workflowId);
      if (jobNumber === -1) {
        return;
      }
      const jobUrl = `https://circleci.com/gh/electron/electron/${jobNumber}`;
      console.log(`CircleCI release build request for ${workflowName} successful.  Check ${jobUrl} for status.`);
    }
  } catch (err) {
    console.log('Error calling CircleCI: ', err);
  }
}

async function getCircleCIWorkflowId (pipelineId) {
  const pipelineInfoUrl = `https://circleci.com/api/v2/pipeline/${pipelineId}`;
  let workflowId = 0;
  while (workflowId === 0) {
    const pipelineInfo = await circleCIRequest(pipelineInfoUrl, 'GET');
    switch (pipelineInfo.state) {
      case 'created': {
        const workflows = await circleCIRequest(`${pipelineInfoUrl}/workflow`, 'GET');
        // The logic below expects three workflow.items: publish, lint, & setup
        if (workflows.items.length === 3) {
          workflowId = workflows.items.find(item => item.name.includes('publish')).id;
          break;
        }
        console.log('Unexpected number of workflows, response was:', workflows);
        workflowId = -1;
        break;
      }
      case 'error': {
        console.log('Error retrieving workflows, response was:', pipelineInfo);
        workflowId = -1;
        break;
      }
    }
    await new Promise(resolve => setTimeout(resolve, CIRCLECI_WAIT_TIME));
  }
  return workflowId;
}

async function getCircleCIJobNumber (workflowId) {
  const jobInfoUrl = `https://circleci.com/api/v2/workflow/${workflowId}/job`;
  let jobNumber = 0;
  while (jobNumber === 0) {
    const jobInfo = await circleCIRequest(jobInfoUrl, 'GET');
    if (!jobInfo.items) {
      continue;
    }
    if (jobInfo.items.length !== 1) {
      console.log('Unexpected number of jobs, response was:', jobInfo);
      jobNumber = -1;
      break;
    }

    switch (jobInfo.items[0].status) {
      case 'not_running':
      case 'queued':
      case 'running': {
        if (jobInfo.items[0].job_number && !isNaN(jobInfo.items[0].job_number)) {
          jobNumber = jobInfo.items[0].job_number;
        }
        break;
      }
      case 'canceled':
      case 'error':
      case 'infrastructure_fail':
      case 'timedout':
      case 'not_run':
      case 'failed': {
        console.log(`Error job returned a status of ${jobInfo.items[0].status}, response was:`, jobInfo);
        jobNumber = -1;
        break;
      }
    }
    await new Promise(resolve => setTimeout(resolve, CIRCLECI_WAIT_TIME));
  }
  return jobNumber;
}

async function getGitHubActionsRun (workflowId, headCommit) {
  let runNumber = 0;
  let actionRun;
  while (runNumber === 0) {
    const actionsRuns = await octokit.request(`GET ${GH_ACTIONS_API_URL}/workflows/${workflowId}.yml/runs`, {
      headers: {
        'X-GitHub-Api-Version': '2022-11-28'
      }
    });
    if (!actionsRuns.data.workflow_runs.length) {
      console.log(`No current workflow_runs found for ${workflowId}, response was: ${actionsRuns.data.workflow_runs}`);
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

async function circleCIRequest (url, method, requestBody) {
  const requestOpts = {
    username: process.env.CIRCLE_TOKEN,
    password: '',
    method,
    url,
    headers: {
      'Content-Type': 'application/json',
      Accept: 'application/json'
    }
  };
  if (requestBody) {
    requestOpts.body = JSON.stringify(requestBody);
  }

  return makeRequest(requestOpts, true).catch(err => {
    if (err.response?.body) {
      console.error('Could not call CircleCI: ', {
        statusCode: err.response.statusCode,
        body: JSON.parse(err.response.body)
      });
    } else {
      console.error('Error calling CircleCI:', err);
    }
  });
}

async function callAppVeyor (targetBranch, job, options) {
  console.log(`Triggering AppVeyor to run build job: ${job} on branch: ${targetBranch} with release flag.`);
  const environmentVariables = {
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
  };
  jobRequestedCount++;

  try {
    const { version } = await makeRequest(requestOpts, true);
    const buildUrl = `https://ci.appveyor.com/project/electron-bot/${appVeyorJobs[job]}/build/${version}`;
    console.log(`AppVeyor release build request for ${job} successful.  Check build status at ${buildUrl}`);
  } catch (err) {
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

function buildAppVeyor (targetBranch, options) {
  const validJobs = Object.keys(appVeyorJobs);
  if (options.job) {
    assert(validJobs.includes(options.job), `Unknown AppVeyor CI job name: ${options.job}.  Valid values are: ${validJobs}.`);
    callAppVeyor(targetBranch, options.job, options);
  } else {
    for (const job of validJobs) {
      callAppVeyor(targetBranch, job, options);
    }
  }
}

function buildCircleCI (targetBranch, options) {
  if (options.job) {
    assert(circleCIPublishWorkflows.includes(options.job), `Unknown CircleCI workflow name: ${options.job}. Valid values are: ${circleCIPublishWorkflows}.`);
    circleCIcall(targetBranch, options.job, options);
  } else {
    assert(!options.arch, 'Cannot provide a single architecture while building all workflows, please specify a single workflow via --workflow');
    options.runningPublishWorkflows = true;
    for (const job of circleCIPublishWorkflows) {
      circleCIcall(targetBranch, job, options);
    }
  }
}

function buildGHActions (targetBranch, options) {
  if (options.job) {
    assert(ghActionsPublishWorkflows.includes(options.job), `Unknown GitHub Actions workflow name: ${options.job}. Valid values are: ${ghActionsPublishWorkflows}.`);
    githubActionsCall(targetBranch, options.job, options);
  } else {
    assert(!options.arch, 'Cannot provide a single architecture while building all workflows, please specify a single workflow via --workflow');
    options.runningPublishWorkflows = true;
    for (const job of ghActionsPublishWorkflows) {
      githubActionsCall(targetBranch, job, options);
    }
  }
}

function runRelease (targetBranch, options) {
  if (options.ci) {
    switch (options.ci) {
      case 'CircleCI': {
        buildCircleCI(targetBranch, options);
        break;
      }
      case 'GitHubActions': {
        buildGHActions(targetBranch, options);
        break;
      }
      case 'AppVeyor': {
        buildAppVeyor(targetBranch, options);
        break;
      }
      default: {
        console.log(`Error! Unknown CI: ${options.ci}.`);
        process.exit(1);
      }
    }
  } else {
    buildCircleCI(targetBranch, options);
    buildAppVeyor(targetBranch, options);
    // TODO(vertedinde): Enable GH Actions in defaults when ready
    // buildGHActions(targetBranch, options);
  }
  console.log(`${jobRequestedCount} jobs were requested.`);
}

module.exports = runRelease;

if (require.main === module) {
  const args = require('minimist')(process.argv.slice(2), {
    boolean: ['ghRelease']
  });
  const targetBranch = args._[0];
  if (args._.length < 1) {
    console.log(`Trigger CI to build release builds of electron.
    Usage: ci-release-build.js [--job=CI_JOB_NAME] [--arch=INDIVIDUAL_ARCH] [--ci=CircleCI|AppVeyor|GitHubActions]
    [--ghRelease] [--circleBuildNum=xxx] [--appveyorJobId=xxx] [--commit=sha] TARGET_BRANCH
    `);
    process.exit(0);
  }
  runRelease(targetBranch, args);
}
