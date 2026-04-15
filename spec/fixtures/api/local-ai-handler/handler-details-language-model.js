const { localAIHandler, LanguageModel } = require('electron/utility');

let callCount = 0;

process.parentPort.on('message', (e) => {
  const { command } = e.data;
  if (command === 'set-null-handler') {
    localAIHandler.setPromptAPIHandler(() => null);
  }
  process.parentPort.postMessage('ack');
});

localAIHandler.setPromptAPIHandler((details) => {
  callCount++;
  process.parentPort.postMessage({ type: 'handler-called', details, callCount });

  return LanguageModel;
});
