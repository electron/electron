'use strict'

const { deprecate } = require('electron')

deprecate.warn(`require('os')`, `remote.require('os')`)

const { getRemoteFor } = require('@electron/internal/renderer/remote')
module.exports = getRemoteFor('os').require('os')
