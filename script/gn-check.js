const sh = require('shelljs')
const cp = require('child_process')

const { getOutDir } = require('./lib/utils')

const OUT_DIR = getOutDir()
if (!OUT_DIR) {
  throw new Error(`No viable out dir: one of Debug, Testing, or Release must exist.`)
}

const gnCheckDirs = [
  '//electron:electron_lib',
  '//electron:electron_app',
  '//electron:manifests',
  '//electron/shell/common/api:mojo'
]

for (const dir of gnCheckDirs) {
  const args = ['check', `../out/${OUT_DIR}`, dir]
  const result = cp.spawnSync('gn', args, { stdio: 'inherit' })
  if (result.status !== 0) sh.exit(result.status)
}

sh.exit(0)
