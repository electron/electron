'use strict'

const { app } = require('electron')
const { EventEmitter } = require('events')
const { systemPreferences, SystemPreferences } = process.atomBinding('system_preferences')

// SystemPreferences is an EventEmitter.
Object.setPrototypeOf(SystemPreferences.prototype, EventEmitter.prototype)
EventEmitter.call(systemPreferences)

if (process.platform === 'darwin') {
  let appearanceTrackingSubscriptionID = null

  systemPreferences.startAppLevelAppearanceTrackingOS = () => {
    if (appearanceTrackingSubscriptionID !== null) return

    const updateAppearanceBasedOnOS = () => {
      const newAppearance = systemPreferences.isDarkMode()
        ? 'dark'
        : 'light'

      if (systemPreferences.getAppLevelAppearance() !== newAppearance) {
        systemPreferences.setAppLevelAppearance(newAppearance)
        // TODO(MarshallOfSound): Once we remove this logic and build against 10.14
        // SDK we should re-implement this event as a monitor of `effectiveAppearance`
        systemPreferences.emit('appearance-changed', newAppearance)
      }
    }

    appearanceTrackingSubscriptionID = systemPreferences.subscribeNotification(
      'AppleInterfaceThemeChangedNotification',
      updateAppearanceBasedOnOS
    )

    updateAppearanceBasedOnOS()
  }

  systemPreferences.stopAppLevelAppearanceTrackingOS = () => {
    if (appearanceTrackingSubscriptionID === null) return

    systemPreferences.unsubscribeNotification(appearanceTrackingSubscriptionID)
  }

  app.on('quit', systemPreferences.stopAppLevelAppearanceTrackingOS)
}

module.exports = systemPreferences
