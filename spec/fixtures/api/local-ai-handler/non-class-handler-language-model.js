const { localAIHandler } = require('electron/utility');

localAIHandler.setPromptAPIHandler(() => {
  return 'not-a-class';
});
