'use strict'

const { deprecate } = require('electron')

deprecate.warn(`require('path')`, `remote.require('path')`)

const { getRemoteForUsage } = require('@electron/internal/renderer/remote')
module.exports = getRemoteForUsage('path').require('path')
