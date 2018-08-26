'use strict'

const { deprecate } = require('electron')

deprecate.warn(`require('os')`, `remote.require('os')`)

const { getRemoteForUsage } = require('@electron/internal/renderer/remote')
module.exports = getRemoteForUsage('os').require('os')
