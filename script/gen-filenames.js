const fs = require('fs')
const path = require('path')

const gniPath = path.resolve(__dirname, '../filenames.auto.gni')

const allDocs = fs.readdirSync(path.resolve(__dirname, '../docs/api'))
  .map(doc => `docs/api/${doc}`)
  .concat(
    fs.readdirSync(path.resolve(__dirname, '../docs/api/structures'))
      .map(doc => `docs/api/structures/${doc}`)
  )

fs.writeFileSync(
  gniPath,
  `# THIS FILE IS AUTO-GENERATED, PLEASE DO NOT EDIT BY HAND
auto_filenames = {
  api_docs = [
${allDocs.map(doc => `    "${doc}",`).join('\n')}
  ]
}
`
)
