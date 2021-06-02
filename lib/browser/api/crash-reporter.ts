import { app, deprecate } from 'electron/main';

const binding = process._linkedBinding('electron_browser_crash_reporter');

class CrashReporter {
  start (options: Electron.CrashReporterStartOptions) {
    const {
      productName = app.name,
      companyName,
      extra = {},
      globalExtra = {},
      ignoreSystemCrashHandler = false,
      submitURL = '',
      uploadToServer = true,
      rateLimit = false,
      compress = true
    } = options || {};

    if (uploadToServer && !submitURL) throw new Error('submitURL must be specified when uploadToServer is true');

    if (!compress && uploadToServer) {
      deprecate.log('Sending uncompressed crash reports is deprecated and will be removed in a future version of Electron. Set { compress: true } to opt-in to the new behavior. Crash reports will be uploaded gzipped, which most crash reporting servers support.');
    }

    const appVersion = app.getVersion();

    if (companyName && globalExtra._companyName == null) globalExtra._companyName = companyName;

    const globalExtraAmended = {
      _productName: productName,
      _version: appVersion,
      ...globalExtra
    };

    binding.start(submitURL, uploadToServer,
      ignoreSystemCrashHandler, rateLimit, compress, globalExtraAmended, extra, false);
  }

  getLastCrashReport () {
    const reports = this.getUploadedReports()
      .sort((a, b) => {
        const ats = (a && a.date) ? new Date(a.date).getTime() : 0;
        const bts = (b && b.date) ? new Date(b.date).getTime() : 0;
        return bts - ats;
      });

    return (reports.length > 0) ? reports[0] : null;
  }

  getUploadedReports (): Electron.CrashReport[] {
    return binding.getUploadedReports();
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
  }

  removeExtraParameter (key: string) {
    binding.removeExtraParameter(key);
  }

  getParameters () {
    return binding.getParameters();
  }
}

export default new CrashReporter();
