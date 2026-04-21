const { localAIHandler, LanguageModel } = require('electron/utility');

const { ReadableStream } = require('node:stream/web');

localAIHandler.setPromptAPIHandler(() => {
  const StreamingLanguageModel = class extends LanguageModel {
    static async create() {
      return new StreamingLanguageModel({
        contextUsage: 0,
        contextWindow: 0
      });
    }

    async prompt() {
      this.contextUsage += 10;

      return new ReadableStream({
        async start(controller) {
          controller.enqueue('Hello ');
          controller.enqueue('World');
          controller.close();
        }
      });
    }
  };

  return StreamingLanguageModel;
});
