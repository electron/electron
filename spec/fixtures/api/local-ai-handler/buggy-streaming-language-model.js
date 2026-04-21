const { localAIHandler, LanguageModel } = require('electron/utility');

const { ReadableStream } = require('node:stream/web');

localAIHandler.setPromptAPIHandler(() => {
  const BuggyStreamingLanguageModel = class extends LanguageModel {
    static async create() {
      return new BuggyStreamingLanguageModel({
        contextUsage: 0,
        contextWindow: 0
      });
    }

    async prompt() {
      this.contextUsage += 10;

      return new ReadableStream({
        async start(controller) {
          controller.enqueue('Hello ');
          controller.enqueue(99);
          controller.close();
        }
      });
    }
  };

  return BuggyStreamingLanguageModel;
});
