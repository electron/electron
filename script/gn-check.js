const sh = require('shelljs')

const { getOutDir } = require('./lib/utils')

const OUT_DIR = getOutDir()
if (!OUT_DIR) {
  throw new Error(`No viable out dir: one of Debug, Testing, or Release must exist.`)
}

const gnCheckString = `
gn check ../out/${OUT_DIR} //electron:electron_lib &&
gn check ../out/${OUT_DIR} //electron:electron_app &&
gn check ../out/${OUT_DIR} //electron:manifests &&
gn check ../out/${OUT_DIR} //electron/shell/common/api:mojo
`
sh.exit(sh.exec(gnCheckString).code)
