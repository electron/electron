interface LanguageModelConstructorValues {
  contextUsage: number;
  contextWindow: number;
}

export default class LanguageModel implements Electron.LanguageModel {
  contextUsage: number;
  contextWindow: number;

  constructor (values: LanguageModelConstructorValues) {
    this.contextUsage = values.contextUsage;
    this.contextWindow = values.contextWindow;
  }

  static async create (): Promise<LanguageModel> {
    return new LanguageModel({
      contextUsage: 0,
      contextWindow: 0
    });
  }

  static async availability () {
    return 'available';
  }

  async prompt () {
    return '';
  }

  async append (): Promise<undefined> {}

  async measureContextUsage () {
    return 0;
  }

  async clone () {
    return new LanguageModel({
      contextUsage: this.contextUsage,
      contextWindow: this.contextWindow
    });
  }

  destroy () {}
}
