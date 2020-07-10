import { invokeSync } from '../ipc-renderer-internal-utils';
import { deprecate } from 'electron';

const binding = process._linkedBinding('electron_renderer_crash_reporter');

export default {
  start (options: Electron.CrashReporterStartOptions) {
    deprecate.log('crashReporter.start is deprecated in the renderer process. Call it from the main process instead.');
    for (const [k, v] of Object.entries(options.extra || {})) {
      binding.addExtraParameter(k, String(v));
    }
  },

  getLastCrashReport (): Electron.CrashReport | null {
    deprecate.log('crashReporter.getLastCrashReport is deprecated in the renderer process. Call it from the main process instead.');
    return invokeSync('ELECTRON_CRASH_REPORTER_GET_LAST_CRASH_REPORT');
  },

  getUploadedReports () {
    deprecate.log('crashReporter.getUploadedReports is deprecated in the renderer process. Call it from the main process instead.');
    return invokeSync('ELECTRON_CRASH_REPORTER_GET_UPLOADED_REPORTS');
  },

  getUploadToServer () {
    deprecate.log('crashReporter.getUploadToServer is deprecated in the renderer process. Call it from the main process instead.');
    return invokeSync('ELECTRON_CRASH_REPORTER_GET_UPLOAD_TO_SERVER');
  },

  setUploadToServer (uploadToServer: boolean) {
    deprecate.log('crashReporter.setUploadToServer is deprecated in the renderer process. Call it from the main process instead.');
    return invokeSync('ELECTRON_CRASH_REPORTER_SET_UPLOAD_TO_SERVER', uploadToServer);
  },

  getCrashesDirectory () {
    deprecate.log('crashReporter.getCrashesDirectory is deprecated in the renderer process. Call it from the main process instead.');
    return invokeSync('ELECTRON_CRASH_REPORTER_GET_CRASHES_DIRECTORY');
  },

  addExtraParameter (key: string, value: string) {
    binding.addExtraParameter(key, value);
  },

  removeExtraParameter (key: string) {
    binding.removeExtraParameter(key);
  },

  getParameters () {
    return binding.getParameters();
  }
};
