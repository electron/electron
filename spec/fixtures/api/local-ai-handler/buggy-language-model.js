const { localAIHandler, LanguageModelUtility } = require('electron/utility');

localAIHandler.setPromptAPIHandler(() => {
  const BuggyLanguageModel = class extends LanguageModelUtility {
    static async create() {
      return 'foobar';
    }

    static availability() {
      return 'foobar';
    }
  };

  return BuggyLanguageModel;
});
