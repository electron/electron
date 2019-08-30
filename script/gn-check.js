const cp = require('child_process')
const path = require('path')

const { getOutDir } = require('./lib/utils')

const SOURCE_ROOT = path.normalize(path.dirname(__dirname))
const DEPOT_TOOLS = path.resolve(SOURCE_ROOT, '..', 'third_party', 'depot_tools')
const OUT_DIR = getOutDir()

if (!OUT_DIR) {
  throw new Error(`No viable out dir: one of Debug, Testing, or Release must exist.`)
}

const env = Object.assign({
  CHROMIUM_BUILDTOOLS_PATH: path.resolve(SOURCE_ROOT, '..', 'buildtools'),
  DEPOT_TOOLS_WIN_TOOLCHAIN: '0'
}, process.env)
// Users may not have depot_tools in PATH.
env.PATH = `${env.PATH}${path.delimiter}${DEPOT_TOOLS}`

const gnCheckDirs = [
  '//electron:electron_lib',
  '//electron:electron_app',
  '//electron:manifests',
  '//electron/shell/common/api:mojo'
]

for (const dir of gnCheckDirs) {
  const args = ['check', `../out/${OUT_DIR}`, dir]
  const result = cp.spawnSync('gn', args, { env, stdio: 'inherit' })
  if (result.status !== 0) process.exit(result.status)
}

process.exit(0)
