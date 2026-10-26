const { localAIHandler, LanguageModelUtility } = require('electron/utility');

localAIHandler.setPromptAPIHandler(() => {
  return Promise.resolve(LanguageModelUtility);
});
