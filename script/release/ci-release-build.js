if (!process.env.CI) require('dotenv-safe').load();

const assert = require('assert');
const got = require('got');
const { Octokit } = require('@octokit/rest');

const BUILD_APPVEYOR_URL = 'https://ci.appveyor.com/api/builds';
const CIRCLECI_PIPELINE_URL = 'https://circleci.com/api/v2/project/gh/electron/electron/pipeline';
const CIRCLECI_WAIT_TIME = process.env.CIRCLECI_WAIT_TIME || 30000;

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

const GHAJobs = ['electron-woa-testing'];

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
    console.log('Error calling CircleCI:', err);
  });
}

function buildAppVeyor (targetBranch, options) {
  const validJobs = Object.keys(appVeyorJobs);
  if (options.job) {
    assert(validJobs.includes(options.job), `Unknown AppVeyor CI job name: ${options.job}.  Valid values are: ${validJobs}.`);
    callAppVeyor(targetBranch, options.job, options);
  } else {
    validJobs.forEach((job) => callAppVeyor(targetBranch, job, options));
  }
}

async function callAppVeyor (targetBranch, job, options) {
  console.log(`Triggering AppVeyor to run build job: ${job} on branch: ${targetBranch} with release flag.`);
  const environmentVariables = {
    ELECTRON_RELEASE: 1,
    APPVEYOR_BUILD_WORKER_CLOUD: 'libcc-20'
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
    console.log('Could not call AppVeyor: ', err);
  }
}

function buildCircleCI (targetBranch, options) {
  if (options.job) {
    assert(circleCIPublishWorkflows.includes(options.job), `Unknown CircleCI workflow name: ${options.job}. Valid values are: ${circleCIPublishWorkflows}.`);
    circleCIcall(targetBranch, options.job, options);
  } else {
    assert(!options.arch, 'Cannot provide a single architecture while building all workflows, please specify a single workflow via --workflow');
    options.runningPublishWorkflows = true;
    circleCIPublishWorkflows.forEach((job) => circleCIcall(targetBranch, job, options));
  }
}

async function buildGHA (targetBranch, options) {
  const { GHA_TOKEN } = process.env;
  assert(GHA_TOKEN, `${options.ci} requires the $GHA_TOKEN environment variable to be provided`);

  const octokit = new Octokit({ auth: GHA_TOKEN });

  assert(GHAJobs.includes(options.job), `Unknown GitHub Actions arm test job name: ${options.job}. Valid values are: ${GHAJobs}.`);
  assert(options.commit !== null, 'commit is a required option for GitHub Actions');

  console.log(`Triggering GitHub Actions to run build on branch: ${targetBranch}.`);

  jobRequestedCount++;

  try {
    const response = await octokit.request('POST /repos/electron/electron/actions/workflows/electron_woa_testing.yml/dispatches', {
      ref: targetBranch,
      inputs: {
        appveyor_job_id: `${options.appveyorJobId}`
      }
    });
  } catch (err) {
    console.log('Problem calling GitHub Actions to get build definitions: ', err);
  }
}

function runRelease (targetBranch, options) {
  if (options.ci) {
    switch (options.ci) {
      case 'CircleCI': {
        buildCircleCI(targetBranch, options);
        break;
      }
      case 'AppVeyor': {
        buildAppVeyor(targetBranch, options);
        break;
      }
      case 'GHA': {
        buildGHA(targetBranch, options);
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
    Usage: ci-release-build.js [--job=CI_JOB_NAME] [--arch=INDIVIDUAL_ARCH] [--ci=CircleCI|AppVeyor|GHA]
    [--ghRelease] [--circleBuildNum=xxx] [--appveyorJobId=xxx] [--commit=sha] TARGET_BRANCH
    `);
    process.exit(0);
  }
  runRelease(targetBranch, args);
}
