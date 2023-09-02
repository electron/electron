import * as deprecate from '@electron/internal/common/deprecate';

const { systemPreferences } = process._linkedBinding('electron_browser_system_preferences');

const invoke = (target: Function, ...args: any[]) => Reflect.apply(target, systemPreferences, args);

if ('getAppLevelAppearance' in systemPreferences) {
  const nativeALAGetter = systemPreferences.getAppLevelAppearance;
  const nativeALASetter = systemPreferences.setAppLevelAppearance;
  const warnALA = deprecate.warnOnce('appLevelAppearance');
  const warnALAGetter = deprecate.warnOnce('getAppLevelAppearance function');
  const warnALASetter = deprecate.warnOnce('setAppLevelAppearance function');
  Object.defineProperty(systemPreferences, 'appLevelAppearance', {
    get: () => {
      warnALA();
      return invoke(nativeALAGetter);
    },
    set: (appearance) => {
      warnALA();
      invoke(nativeALASetter, appearance);
    }
  });
  systemPreferences.getAppLevelAppearance = () => {
    warnALAGetter();
    return invoke(nativeALAGetter);
  };
  systemPreferences.setAppLevelAppearance = (appearance) => {
    warnALASetter();
    invoke(nativeALASetter, appearance);
  };
}

if ('getEffectiveAppearance' in systemPreferences) {
  const nativeEAGetter = systemPreferences.getEffectiveAppearance;
  Object.defineProperty(systemPreferences, 'effectiveAppearance', {
    get: () => invoke(nativeEAGetter)
  });
}

export default systemPreferences;
