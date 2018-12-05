'use strict'

const { deprecate } = require('electron')

deprecate.warn(`require('fs')`, `remote.require('fs')`)

const { getRemoteFor } = require('@electron/internal/renderer/remote')
module.exports = getRemoteFor('fs').require('fs')
