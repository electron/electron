import * as deprecate from '@electron/internal/common/deprecate';

const { systemPreferences } = process._linkedBinding('electron_browser_system_preferences');

if ('getEffectiveAppearance' in systemPreferences) {
  const nativeEAGetter = systemPreferences.getEffectiveAppearance;
  Object.defineProperty(systemPreferences, 'effectiveAppearance', {
    get: () => nativeEAGetter.call(systemPreferences)
  });
}

if ('accessibilityDisplayShouldReduceTransparency' in systemPreferences) {
  const reduceTransparencyDeprecated = deprecate.warnOnce('systemPreferences.accessibilityDisplayShouldReduceTransparency', 'nativeTheme.prefersReducedTransparency');
  const nativeReduceTransparency = systemPreferences.accessibilityDisplayShouldReduceTransparency;
  Object.defineProperty(systemPreferences, 'accessibilityDisplayShouldReduceTransparency', {
    get: () => {
      reduceTransparencyDeprecated();
      return nativeReduceTransparency;
    }
  });
}

export default systemPreferences;
