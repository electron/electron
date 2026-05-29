const { localAIHandler, LanguageModelUtility } = require('electron/utility');

localAIHandler.setPromptAPIHandler(() => {
  const BasicLanguageModel = class extends LanguageModelUtility {
    static async create() {
      return new BasicLanguageModel({
        contextUsage: 0,
        contextWindow: 12345
      });
    }

    async prompt() {
      this.contextUsage += 10;

      return 'foobar';
    }

    async append() {
      this.contextUsage += 5;
    }

    async measureContextUsage() {
      return 42;
    }
  };

  return BasicLanguageModel;
});
