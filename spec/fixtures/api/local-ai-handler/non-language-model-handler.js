const { localAIHandler } = require('electron/utility');

localAIHandler.setPromptAPIHandler(() => {
  return class NotALanguageModel {};
});
