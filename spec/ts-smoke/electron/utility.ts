/* eslint-disable */

import { localAIHandler, net, systemPreferences, LanguageModel } from 'electron/utility';

process.parentPort.on('message', (e) => {
  if (e.data === 'Hello from parent!') {
    process.parentPort.postMessage('Hello from child!');
  }
});

// net
// https://github.com/electron/electron/blob/main/docs/api/net.md

const request = net.request('https://github.com');
request.setHeader('Some-Custom-Header-Name', 'Some-Custom-Header-Value');
const header = request.getHeader('Some-Custom-Header-Name');
console.log('header', header);
request.removeHeader('Some-Custom-Header-Name');
request.on('response', (response) => {
  console.log(`Status code: ${response.statusCode}`);
  console.log(`Status message: ${response.statusMessage}`);
  console.log(`Headers: ${JSON.stringify(response.headers)}`);
  console.log(`Http version: ${response.httpVersion}`);
  console.log(`Major Http version: ${response.httpVersionMajor}`);
  console.log(`Minor Http version: ${response.httpVersionMinor}`);
  response.on('data', (chunk) => {
    console.log(`BODY: ${chunk}`);
  });
  response.on('end', () => {
    console.log('No more data in response.');
  });
  response.on('error', () => {
    console.log('"error" event emitted');
  });
  response.on('aborted', () => {
    console.log('"aborted" event emitted');
  });
});
request.on('login', (authInfo, callback) => {
  callback('username', 'password');
});
request.on('finish', () => {
  console.log('"finish" event emitted');
});
request.on('abort', () => {
  console.log('"abort" event emitted');
});
request.on('error', () => {
  console.log('"error" event emitted');
});
request.write('Hello World!', 'utf-8');
request.end('Hello World!', 'utf-8');
request.abort();

// systemPreferences
// https://github.com/electron/electron/blob/main/docs/api/system-preferences.md

if (process.platform === 'win32') {
  systemPreferences.on('color-changed', () => { console.log('color changed'); });
}

if (process.platform === 'darwin') {
  const value = systemPreferences.getUserDefault('Foo', 'string');
  console.log(value);
  const value2 = systemPreferences.getUserDefault('Foo', 'boolean');
  console.log(value2);
}

// localAIHandler
// https://github.com/electron/electron/blob/main/docs/api/local-ai-handler.md

localAIHandler.setPromptAPIHandler(async (details) => {
  return class MyLanguageModel extends LanguageModel {
    private details = details;

    static async create() {
      return new MyLanguageModel({
        inputUsage: 0,
        inputQuota: 0,
        topK: 7,
        temperature: 0
      })
    }

    static async params () {
      return {
          defaultTopK: 12,
          defaultTemperature: 0.3,
          maxTopK: 23,
          maxTemperature: 0.5
      }
    }

    async prompt () {
        return "Hello World"
    }
  }
})

localAIHandler.setPromptAPIHandler(null);
