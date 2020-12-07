const { systemPreferences } = process._linkedBinding('electron_browser_system_preferences');

if ('getAppLevelAppearance' in systemPreferences) {
  const nativeALAGetter = systemPreferences.getAppLevelAppearance;
  const nativeALASetter = systemPreferences.setAppLevelAppearance;
  Object.defineProperty(systemPreferences, 'appLevelAppearance', {
    get: () => nativeALAGetter.call(systemPreferences),
    set: (appearance) => nativeALASetter.call(systemPreferences, appearance)
  });
}

if ('getEffectiveAppearance' in systemPreferences) {
  const nativeEAGetter = systemPreferences.getEffectiveAppearance;
  Object.defineProperty(systemPreferences, 'effectiveAppearance', {
    get: () => nativeEAGetter.call(systemPreferences)
  });
}

export default systemPreferences;
