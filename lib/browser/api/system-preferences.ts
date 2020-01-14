import { EventEmitter } from 'events'
import { deprecate } from 'electron'
const { systemPreferences, SystemPreferences } = process.electronBinding('system_preferences')

// SystemPreferences is an EventEmitter.
Object.setPrototypeOf(SystemPreferences.prototype, EventEmitter.prototype)
EventEmitter.call(systemPreferences)

if ('appLevelAppearance' in systemPreferences) {
  deprecate.fnToProperty(
    SystemPreferences.prototype,
    'appLevelAppearance',
    '_getAppLevelAppearance',
    '_setAppLevelAppearance'
  )
}

if ('effectiveAppearance' in systemPreferences) {
  deprecate.fnToProperty(
    SystemPreferences.prototype,
    'effectiveAppearance',
    '_getEffectiveAppearance'
  )
}

SystemPreferences.prototype.isDarkMode = deprecate.moveAPI(
  SystemPreferences.prototype.isDarkMode,
  'systemPreferences.isDarkMode()',
  'nativeTheme.shouldUseDarkColors'
)
SystemPreferences.prototype.isInvertedColorScheme = deprecate.moveAPI(
  SystemPreferences.prototype.isInvertedColorScheme,
  'systemPreferences.isInvertedColorScheme()',
  'nativeTheme.shouldUseInvertedColorScheme'
)
SystemPreferences.prototype.isHighContrastColorScheme = deprecate.moveAPI(
  SystemPreferences.prototype.isHighContrastColorScheme,
  'systemPreferences.isHighContrastColorScheme()',
  'nativeTheme.shouldUseHighContrastColors'
)

module.exports = systemPreferences
