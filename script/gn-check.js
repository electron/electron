const sh = require('shelljs')
const path = require('path')
const fs = require('fs')

const SRC_DIR = path.resolve(__dirname, '..', '..')
let GN_CHECK_DIR

if (process.env.ELECTRON_OUT_DIR) {
  GN_CHECK_DIR = `../out/${process.env.ELECTRON_OUT_DIR}`
} else {
  for (const buildType of ['Debug', 'Testing', 'Release']) {
    const outPath = path.resolve(SRC_DIR, 'out', buildType)
    if (fs.existsSync(outPath)) {
      GN_CHECK_DIR = `../out/${buildType}`
      break
    }
  }
}

const gnCheckString = `gn check ${GN_CHECK_DIR} //electron:electron_lib`
sh.exit(sh.exec(gnCheckString).code)
