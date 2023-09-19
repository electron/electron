const fs = require('fs');

const PARAMS_PATH = '/tmp/pipeline-parameters.json';

const content = JSON.parse(fs.readFileSync(PARAMS_PATH, 'utf-8'));

// Choose resource class for linux hosts
const currentBranch = process.env.CIRCLE_BRANCH || '';
content['large-linux-executor'] = /^pull\/[0-9-]+$/.test(currentBranch) ? '2xlarge' : 'electronjs/aks-linux-large';
content['medium-linux-executor'] = /^pull\/[0-9-]+$/.test(currentBranch) ? 'medium' : 'electronjs/aks-linux-medium';

fs.writeFileSync(PARAMS_PATH, JSON.stringify(content));
