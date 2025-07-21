interface LanguageModelConstructorValues {
  inputUsage: number;
  inputQuota: number;
  topK: number;
  temperature: number;
}

export default class LanguageModel implements Electron.LanguageModel {
  readonly inputUsage: number;
  readonly inputQuota: number;
  readonly topK: number;
  readonly temperature: number;

  constructor (values: LanguageModelConstructorValues) {
    this.inputUsage = values.inputUsage;
    this.inputQuota = values.inputQuota;
    this.topK = values.topK;
    this.temperature = values.temperature;
  }

  static async create (): Promise<LanguageModel> {
    return new LanguageModel({
      inputUsage: 0,
      inputQuota: 0,
      topK: 0,
      temperature: 0
    });
  }

  static async availability () {
    return 'available';
  }

  static async params () {
    return null;
  }

  async prompt () {
    return '';
  }

  async append () {}

  async measureInputUsage () {
    return 0;
  }

  async clone () {
    return new LanguageModel({
      inputUsage: this.inputUsage,
      inputQuota: this.inputQuota,
      topK: this.topK,
      temperature: this.temperature
    });
  }

  destroy () {}
}
