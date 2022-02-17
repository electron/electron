const AWS = require('aws-sdk');

const lambda = new AWS.Lambda({
  credentials: {
    accessKeyId: process.env.AWS_LAMBDA_EXECUTE_KEY,
    secretAccessKey: process.env.AWS_LAMBDA_EXECUTE_SECRET
  },
  region: 'us-east-1'
});

module.exports = async function getUrlHash (targetUrl, algorithm = 'sha256', attempts = 3) {
  try {
    return new Promise((resolve, reject) => {
      lambda.invoke({
        FunctionName: 'hasher',
        Payload: JSON.stringify({
          targetUrl,
          algorithm
        })
      }, (err, data) => {
        if (err) return reject(err);
        try {
          const response = JSON.parse(data.Payload);
          if (response.statusCode !== 200) return reject(new Error('non-200 status code received from hasher function'));
          if (!response.hash) return reject(new Error('Successful lambda call but failed to get valid hash'));
          resolve(response.hash);
        } catch (err) {
          return reject(err);
        }
      });
    });
  } catch (err) {
    if (attempts > 1) {
      console.error('Failed to get URL hash for', targetUrl, 'we will retry', err);
      return getUrlHash(targetUrl, algorithm, attempts - 1);
    }
    throw err;
  }
};
