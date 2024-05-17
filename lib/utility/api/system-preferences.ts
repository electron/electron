const { systemPreferences } = process._linkedBinding('electron_browser_system_preferences');

if ('getEffectiveAppearance' in systemPreferences) {
  const nativeEAGetter = systemPreferences.getEffectiveAppearance;
  Object.defineProperty(systemPreferences, 'effectiveAppearance', {
    get: () => nativeEAGetter.call(systemPreferences)
  });
}

export default systemPreferences;
