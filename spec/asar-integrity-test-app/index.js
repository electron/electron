'use strict'

const { app } = require('electron')
const { join } = require('path')

app.on('ready', () => {
  if (process.env.TEST_REQUIRE_EXTERNAL) {
    require(join(process.resourcesPath, '..', 'external.asar'))
  }

  process.exit(0)
})
