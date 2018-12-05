'use strict'

const { deprecate } = require('electron')

deprecate.warn(`require('path')`, `remote.require('path')`)

const { getRemoteFor } = require('@electron/internal/renderer/remote')
module.exports = getRemoteFor('path').require('path')
