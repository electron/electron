const { localAIHandler, LanguageModel } = require('electron/utility');

const { once } = require('node:events');

let availabilityState = 'available';
let createResponse = null;
let promptResponse = 'Hello World';
let appendResponse = null;
let measureResponse = 100;
let cloneResponse = null;

process.parentPort.on('message', (e) => {
  const { command, value } = e.data;
  if (command === 'set-availability') {
    availabilityState = value;
  } else if (command === 'set-create') {
    createResponse = value;
  } else if (command === 'set-prompt-response') {
    promptResponse = value;
  } else if (command === 'set-append-response') {
    appendResponse = value;
  } else if (command === 'set-measure-response') {
    measureResponse = value;
  } else if (command === 'set-clone-response') {
    cloneResponse = value;
  }

  process.parentPort.postMessage('ack');
});

async function waitForAbort (signal, messageType) {
  await once(signal, 'abort');
  process.parentPort.postMessage({ type: messageType });
  throw new Error('Aborted');
}

localAIHandler.setPromptAPIHandler(() => {
  const ControllableLanguageModel = class extends LanguageModel {
    static async create (options) {
      process.parentPort.postMessage({ type: 'create-called', options });
      if (createResponse === 'reject') {
        return Promise.reject(new Error('Model is unavailable'));
      } else if (createResponse === 'wait-for-abort') {
        await waitForAbort(options.signal, 'create-aborted');
      }
      return new ControllableLanguageModel({
        contextUsage: 0,
        contextWindow: 0
      });
    }

    static async availability (options) {
      process.parentPort.postMessage({ type: 'availability-called', options });
      if (availabilityState === 'reject') {
        return Promise.reject(new Error('Model is unavailable'));
      }
      return availabilityState;
    }

    async prompt (input, options) {
      process.parentPort.postMessage({ type: 'prompt-called', input, options });
      if (promptResponse === 'reject') {
        return Promise.reject(new Error('Model is unavailable'));
      } else if (promptResponse === 'wait-for-abort') {
        await waitForAbort(options.signal, 'prompt-aborted');
      }
      return promptResponse;
    }

    async append (input, options) {
      process.parentPort.postMessage({ type: 'append-called', input, options });
      if (appendResponse === 'reject') {
        return Promise.reject(new Error('Append failed'));
      } else if (appendResponse === 'wait-for-abort') {
        await waitForAbort(options.signal, 'append-aborted');
      }
    }

    async measureContextUsage (input, options) {
      process.parentPort.postMessage({ type: 'measure-called', input, options });
      if (measureResponse === 'reject') {
        return Promise.reject(new Error('Measure failed'));
      } else if (measureResponse === 'wait-for-abort') {
        await waitForAbort(options.signal, 'measure-aborted');
      }
      return measureResponse;
    }

    async clone (options) {
      process.parentPort.postMessage({ type: 'clone-called', options });
      if (cloneResponse === 'reject') {
        return Promise.reject(new Error('Clone failed'));
      } else if (cloneResponse === 'wait-for-abort') {
        await waitForAbort(options.signal, 'clone-aborted');
      } else if (cloneResponse === 'invalid') {
        return 'not-a-language-model';
      }
      return new ControllableLanguageModel({
        contextUsage: this.contextUsage,
        contextWindow: this.contextWindow
      });
    }

    destroy () {
      process.parentPort.postMessage({ type: 'destroy-called' });
    }
  };

  return ControllableLanguageModel;
});
