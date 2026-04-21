const { localAIHandler, LanguageModel } = require('electron/utility');

function sendInProgressPromptHandlerCallMessage() {
  process.parentPort.postMessage({ type: 'in-progress-prompt-handler-call' });
}

localAIHandler.on('-can-create-language-model', sendInProgressPromptHandlerCallMessage);
localAIHandler.on('-pending-bind-ai-manager', sendInProgressPromptHandlerCallMessage);

process.parentPort.on('message', (e) => {
  const { command } = e.data;
  if (command === 'set-handler') {
    localAIHandler.setPromptAPIHandler(() => {
      return LanguageModel;
    });
  }

  process.parentPort.postMessage('ack');
});
