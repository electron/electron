'use strict'

const { deprecate } = require('electron')

deprecate.warn(`require('fs')`, `remote.require('fs')`)

const { getRemoteForUsage } = require('@electron/internal/renderer/remote')
module.exports = getRemoteForUsage('fs').require('fs')
