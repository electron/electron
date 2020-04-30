import { app } from 'electron';

const binding = process.electronBinding('crash_reporter');

const getTempDirectory = function () {
  try {
    return app.getPath('temp');
  } catch {
    // Delibrately lazy-load the os module, this file is on the hot path when
    // booting Electron and os takes between 5 - 8ms to load and we do not need
    // it yet.
    return require('os').tmpdir();
  }
};

class CrashReporter {
  _crashesDirectory: string | null = null;
  _extra: Record<string, string> = {};

  start (options: Electron.CrashReporterStartOptions) {
    const {
      productName = app.name,
      companyName,
      extra = {},
      globalExtra = {},
      ignoreSystemCrashHandler = false,
      submitURL,
      uploadToServer = true,
      rateLimit = false,
      compress = false
    } = options || {};

    if (submitURL == null) throw new Error('submitURL is a required option to crashReporter.start');

    this._crashesDirectory = require('path').join(getTempDirectory(), `${productName} Crashes`);
    const appVersion = app.getVersion();

    if (companyName && globalExtra._companyName == null) globalExtra._companyName = companyName;

    if (process.platform === 'linux') {
      // Linux (breakpad) doesn't allow fetching the value of the crash keys in
      // C++, so we shim it here.
      for (const [k, v] of Object.entries(extra)) {
        this._extra[k] = String(v);
      }
    }

    const extraGlobal = {
      _productName: productName,
      _version: appVersion,
      ...globalExtra
    };

    binding.start(submitURL, this._crashesDirectory, uploadToServer,
      ignoreSystemCrashHandler, rateLimit, compress, extraGlobal, extra);
  }

  async getLastCrashReport () {
    const reports = (await this.getUploadedReports())
      .sort((a, b) => {
        const ats = (a && a.date) ? new Date(a.date).getTime() : 0;
        const bts = (b && b.date) ? new Date(b.date).getTime() : 0;
        return bts - ats;
      });

    return (reports.length > 0) ? reports[0] : null;
  }

  getUploadedReports (): Promise<Electron.CrashReport[]> {
    return new Promise(resolve => binding.getUploadedReports(resolve));
  }

  getCrashesDirectory () {
    return this._crashesDirectory;
  }

  getUploadToServer () {
    if (process.type === 'browser') {
      return binding.getUploadToServer();
    } else {
      throw new Error('getUploadToServer can only be called from the main process');
    }
  }

  setUploadToServer (uploadToServer: boolean) {
    if (process.type === 'browser') {
      return binding.setUploadToServer(uploadToServer);
    } else {
      throw new Error('setUploadToServer can only be called from the main process');
    }
  }

  addExtraParameter (key: string, value: string) {
    binding.addExtraParameter(key, value);
    if (process.platform === 'linux') { this._extra[key] = String(value); }
  }

  removeExtraParameter (key: string) {
    binding.removeExtraParameter(key);
    if (process.platform === 'linux') { delete this._extra[key]; }
  }

  getParameters () {
    if (process.platform === 'linux') { return { ...this._extra }; } else { return binding.getParameters(); }
  }
}

export default new CrashReporter();
