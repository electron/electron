if (!process.env.CI) require('dotenv-safe').load();

const assert = require('assert');
const request = require('request');

const BUILD_APPVEYOR_URL = 'https://ci.appveyor.com/api/builds';
const CIRCLECI_PIPELINE_URL = 'https://circleci.com/api/v2/project/gh/electron/electron/pipeline';
const VSTS_URL = 'https://github.visualstudio.com/electron/_apis/build';
const DEVOPS_URL = 'https://dev.azure.com/electron-ci/electron/_apis/build';
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

const circleCIJobs = circleCIPublishWorkflows.concat([
  'linux-arm-publish',
  'linux-arm64-publish',
  'linux-ia32-publish',
  'linux-x64-publish',
  'mas-publish',
  'mas-publish-arm64',
  'osx-publish',
  'osx-publish-arm64'
]);

const vstsArmJobs = [
  'electron-arm-testing',
  'electron-osx-arm64-testing',
  'electron-mas-arm64-testing',
  'electron-arm64-testing',
  'electron-woa-testing'
];

let jobRequestedCount = 0;

async function makeRequest (requestOptions, parseResponse) {
  return new Promise((resolve, reject) => {
    request(requestOptions, (err, res, body) => {
      if (!err && res.statusCode >= 200 && res.statusCode < 300) {
        if (parseResponse) {
          const build = JSON.parse(body);
          resolve(build);
        } else {
          resolve(body);
        }
      } else {
        console.error('Error occurred while requesting:', requestOptions.url);
        if (parseResponse) {
          try {
            console.log('Error: ', `(status ${res.statusCode})`, err || JSON.parse(res.body));
          } catch (err) {
            console.log('Error: ', `(status ${res.statusCode})`, res.body);
          }
        } else {
          console.log('Error: ', `(status ${res.statusCode})`, err || res.body);
        }
        reject(err);
      }
    });
  });
}

async function circleCIcall (targetBranch, job, options) {
  console.log(`Triggering CircleCI to run build job: ${job} on branch: ${targetBranch} with release flag.`);
  const buildRequest = {
    branch: targetBranch,
    parameters: {}
  };
  if (options.ghRelease) {
    buildRequest.parameters['upload-to-s3'] = '0';
  } else {
    buildRequest.parameters['upload-to-s3'] = '1';
  }
  buildRequest.parameters[`run-${job}`] = true;
  jobRequestedCount++;
  // The logic below expects that the CircleCI workflows for releases each
  // contain only one job in order to maintain compatibility with sudowoodo.
  // If the workflows are changed in the CircleCI config.yml, this logic will
  // also need to be changed as well as possibly changing sudowoodo.
  try {
    const circleResponse = await circleCIRequest(CIRCLECI_PIPELINE_URL, 'POST', buildRequest);
    console.log(`CircleCI release build pipeline ${circleResponse.id} for ${job} triggered.`);
    const workflowId = await getCircleCIWorkflowId(circleResponse.id);
    if (workflowId === -1) {
      return;
    }
    const workFlowUrl = `https://circleci.com/workflow-run/${workflowId}`;
    if (options.runningPublishWorkflows) {
      console.log(`CircleCI release workflow request for ${job} successful.  Check ${workFlowUrl} for status.`);
    } else {
      console.log(`CircleCI release build workflow running at https://circleci.com/workflow-run/${workflowId} for ${job}.`);
      const jobNumber = await getCircleCIJobNumber(workflowId);
      if (jobNumber === -1) {
        return;
      }
      const jobUrl = `https://circleci.com/gh/electron/electron/${jobNumber}`;
      console.log(`CircleCI release build request for ${job} successful.  Check ${jobUrl} for status.`);
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
        if (workflows.items.length === 1) {
          workflowId = workflows.items[0].id;
          break;
        }
        console.log('Unxpected number of workflows, response was:', pipelineInfo);
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
      console.log('Unxpected number of jobs, response was:', jobInfo);
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
  return makeRequest({
    auth: {
      username: process.env.CIRCLE_TOKEN,
      password: ''
    },
    method,
    url,
    headers: {
      'Content-Type': 'application/json',
      Accept: 'application/json'
    },
    body: requestBody ? JSON.stringify(requestBody) : null
  }, true).catch(err => {
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
    environmentVariables.UPLOAD_TO_S3 = 1;
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
    assert(circleCIJobs.includes(options.job), `Unknown CircleCI job name: ${options.job}. Valid values are: ${circleCIJobs}.`);
    circleCIcall(targetBranch, options.job, options);
  } else {
    options.runningPublishWorkflows = true;
    circleCIPublishWorkflows.forEach((job) => circleCIcall(targetBranch, job, options));
  }
}

async function buildVSTS (targetBranch, options) {
  assert(options.armTest, `${options.ci} only works with the --armTest option.`);
  assert(vstsArmJobs.includes(options.job), `Unknown VSTS CI arm test job name: ${options.job}. Valid values are: ${vstsArmJobs}.`);

  console.log(`Triggering VSTS to run build on branch: ${targetBranch}.`);
  const environmentVariables = {};

  if (options.circleBuildNum) {
    environmentVariables.CIRCLE_BUILD_NUM = options.circleBuildNum;
  } else if (options.appveyorJobId) {
    environmentVariables.APPVEYOR_JOB_ID = options.appveyorJobId;
  }

  let vstsURL = VSTS_URL;
  let vstsToken = process.env.VSTS_TOKEN;
  assert(vstsToken, `${options.ci} requires the $VSTS_TOKEN environment variable to be provided`);
  if (options.ci === 'DevOps') {
    vstsURL = DEVOPS_URL;
    vstsToken = process.env.DEVOPS_TOKEN;
  }
  const requestOpts = {
    url: `${vstsURL}/definitions?api-version=4.1`,
    auth: {
      user: '',
      password: vstsToken
    },
    headers: {
      'Content-Type': 'application/json'
    }
  };

  jobRequestedCount++;

  try {
    const vstsResponse = await makeRequest(requestOpts, true);
    const buildToRun = vstsResponse.value.find(build => build.name === options.job);
    callVSTSBuild(buildToRun, targetBranch, environmentVariables, vstsURL, vstsToken);
  } catch (err) {
    console.log('Problem calling VSTS to get build definitions: ', err);
  }
}

async function callVSTSBuild (build, targetBranch, environmentVariables, vstsURL, vstsToken) {
  const buildBody = {
    definition: build,
    sourceBranch: targetBranch,
    priority: 'high'
  };
  if (Object.keys(environmentVariables).length !== 0) {
    buildBody.parameters = JSON.stringify(environmentVariables);
  }
  const requestOpts = {
    url: `${vstsURL}/builds?api-version=4.1`,
    auth: {
      user: '',
      password: vstsToken
    },
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(buildBody),
    method: 'POST'
  };

  try {
    const { _links } = await makeRequest(requestOpts, true);
    console.log(`VSTS release build request for ${build.name} successful. Check ${_links.web.href} for status.`);
  } catch (err) {
    console.log(`Could not call VSTS for job ${build.name}: `, err);
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
      case 'DevOps':
      case 'VSTS': {
        buildVSTS(targetBranch, options);
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
    boolean: ['ghRelease', 'armTest']
  });
  const targetBranch = args._[0];
  if (args._.length < 1) {
    console.log(`Trigger CI to build release builds of electron.
    Usage: ci-release-build.js [--job=CI_JOB_NAME] [--ci=CircleCI|AppVeyor|VSTS|DevOps]
    [--ghRelease] [--armTest] [--circleBuildNum=xxx] [--appveyorJobId=xxx] [--commit=sha] TARGET_BRANCH
    `);
    process.exit(0);
  }
  runRelease(targetBranch, args);
}
