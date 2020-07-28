import { EventEmitter } from 'events';
import { deprecate } from 'electron/main';
const { systemPreferences, SystemPreferences } = process._linkedBinding('electron_browser_system_preferences');

// SystemPreferences is an EventEmitter.
Object.setPrototypeOf(SystemPreferences.prototype, EventEmitter.prototype);
EventEmitter.call(systemPreferences);

if ('getAppLevelAppearance' in systemPreferences) {
  const nativeALAGetter = systemPreferences.getAppLevelAppearance;
  const nativeALASetter = systemPreferences.setAppLevelAppearance;
  Object.defineProperty(SystemPreferences.prototype, 'appLevelAppearance', {
    get: () => nativeALAGetter.call(systemPreferences),
    set: (appearance) => nativeALASetter.call(systemPreferences, appearance)
  });
}

if ('getEffectiveAppearance' in systemPreferences) {
  const nativeEAGetter = systemPreferences.getAppLevelAppearance;
  Object.defineProperty(SystemPreferences.prototype, 'effectiveAppearance', {
    get: () => nativeEAGetter.call(systemPreferences)
  });
}

SystemPreferences.prototype.isDarkMode = deprecate.moveAPI(
  SystemPreferences.prototype.isDarkMode,
  'systemPreferences.isDarkMode()',
  'nativeTheme.shouldUseDarkColors'
);
SystemPreferences.prototype.isInvertedColorScheme = deprecate.moveAPI(
  SystemPreferences.prototype.isInvertedColorScheme,
  'systemPreferences.isInvertedColorScheme()',
  'nativeTheme.shouldUseInvertedColorScheme'
);
SystemPreferences.prototype.isHighContrastColorScheme = deprecate.moveAPI(
  SystemPreferences.prototype.isHighContrastColorScheme,
  'systemPreferences.isHighContrastColorScheme()',
  'nativeTheme.shouldUseHighContrastColors'
);

module.exports = systemPreferences;
