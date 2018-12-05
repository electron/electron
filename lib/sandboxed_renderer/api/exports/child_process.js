'use strict'

const { deprecate } = require('electron')

deprecate.warn(`require('child_process')`, `remote.require('child_process')`)

const { getRemoteFor } = require('@electron/internal/renderer/remote')
module.exports = getRemoteFor('child_process').require('child_process')
