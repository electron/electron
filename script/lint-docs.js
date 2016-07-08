#!/usr/bin/env node

const fs = require('fs')
const path = require('path')
const lint = require('electron-docs-linter')
const args = process.argv.slice(2)
const docsPath = path.join(__dirname, '../docs/api')
const targetVersion = args[0]
const outFile = path.join(__dirname, '../electron.json')

if (!targetVersion) {
  console.error(`Usage: ./script/lint-docs.js <targetVersion>`)
  process.exit(1)
}

lint(docsPath, targetVersion).then(function(apis) {
  fs.writeFileSync(outFile, JSON.stringify(apis, null, 2))
  console.log(`Wrote ${outFile}`)
})
