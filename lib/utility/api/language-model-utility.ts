interface LanguageModelConstructorValues {
  contextUsage: number;
  contextWindow: number;
}

export default class LanguageModelUtility implements Electron.LanguageModelUtility {
  contextUsage: number;
  contextWindow: number;

  constructor(values: LanguageModelConstructorValues) {
    this.contextUsage = values.contextUsage;
    this.contextWindow = values.contextWindow;
  }

  static async create(): Promise<LanguageModelUtility> {
    return new LanguageModelUtility({
      contextUsage: 0,
      contextWindow: 0
    });
  }

  static async availability() {
    return 'available';
  }

  async prompt() {
    return '';
  }

  async append(): Promise<undefined> {}

  async measureContextUsage() {
    return 0;
  }

  async clone() {
    return new LanguageModelUtility({
      contextUsage: this.contextUsage,
      contextWindow: this.contextWindow
    });
  }

  destroy() {}
}
