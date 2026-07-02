const { localAIHandler, LanguageModelUtility } = require('electron/utility');

localAIHandler.setPromptAPIHandler(() => {
  return LanguageModelUtility;
});
