'use strict'

const { deprecate } = require('electron')

deprecate.warn(`electron.screen`, `electron.remote.screen`)

const { getRemoteForUsage } = require('@electron/internal/renderer/remote')
module.exports = getRemoteForUsage('screen').screen
