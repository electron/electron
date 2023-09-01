import * as deprecate from '@electron/internal/common/deprecate';

const { systemPreferences } = process._linkedBinding('electron_browser_system_preferences');

if ('getAppLevelAppearance' in systemPreferences) {
  const nativeALAGetter = systemPreferences.getAppLevelAppearance;
  const nativeALASetter = systemPreferences.setAppLevelAppearance;
  const warnALA = deprecate.warnOnce('appLevelAppearance');
  const warnALAGetter = deprecate.warnOnce('getAppLevelAppearance function');
  const warnALASetter = deprecate.warnOnce('setAppLevelAppearance function');
  Object.defineProperty(systemPreferences, 'appLevelAppearance', {
    get: () => {
      warnALA();
      return nativeALAGetter.call(systemPreferences);
    },
    set: (appearance) => {
      warnALA();
      nativeALASetter.call(systemPreferences, appearance);
    }
  });
  systemPreferences.getAppLevelAppearance = () => {
    warnALAGetter();
    return nativeALAGetter.call(systemPreferences);
  };
  systemPreferences.setAppLevelAppearance = (appearance) => {
    warnALASetter();
    nativeALASetter.call(systemPreferences, appearance);
  };
}

if ('getEffectiveAppearance' in systemPreferences) {
  const nativeEAGetter = systemPreferences.getEffectiveAppearance;
  Object.defineProperty(systemPreferences, 'effectiveAppearance', {
    get: () => nativeEAGetter.call(systemPreferences)
  });
}

export default systemPreferences;
