const { localAIHandler, LanguageModel } = require('electron/utility');

localAIHandler.setPromptAPIHandler(() => {
  return LanguageModel;
});
