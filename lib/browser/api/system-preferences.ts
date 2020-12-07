import { defineProperty } from '@electron/internal/common/property-utils';

const { systemPreferences } = process._linkedBinding('electron_browser_system_preferences');

if ('getAppLevelAppearance' in systemPreferences) {
  defineProperty(systemPreferences, 'appLevelAppearance', systemPreferences.getAppLevelAppearance, systemPreferences.setAppLevelAppearance);
}

if ('getEffectiveAppearance' in systemPreferences) {
  defineProperty(systemPreferences, 'effectiveAppearance', systemPreferences.getEffectiveAppearance);
}

export default systemPreferences;
