'use strict'

const { deprecate } = require('electron')

deprecate.warn(`require('child_process')`, `remote.require('child_process')`)

const { getRemoteForUsage } = require('@electron/internal/renderer/remote')
module.exports = getRemoteForUsage('child_process').require('child_process')
