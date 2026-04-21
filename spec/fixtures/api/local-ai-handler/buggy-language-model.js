const { localAIHandler, LanguageModel } = require('electron/utility');

localAIHandler.setPromptAPIHandler(() => {
  const BuggyLanguageModel = class extends LanguageModel {
    static async create() {
      return 'foobar';
    }

    static availability() {
      return 'foobar';
    }
  };

  return BuggyLanguageModel;
});
