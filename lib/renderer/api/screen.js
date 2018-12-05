'use strict'

const { deprecate } = require('electron')

deprecate.warn(`electron.screen`, `electron.remote.screen`)

const { getRemote } = require('@electron/internal/renderer/remote')
module.exports = getRemote('screen')
