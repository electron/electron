bindings = process.atomBinding 'power_save_blocker'

powerSaveBlocker = bindings.powerSaveBlocker

powerSaveBlocker.PREVENT_APP_SUSPENSION = 0
powerSaveBlocker.PREVENT_DISPLAY_SLEEP = 1

module.exports = powerSaveBlocker
