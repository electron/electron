const fs = require('fs-extra')
const path = require('path')

const DEBUG_PATH = path.join(__dirname, '../../out/Debug')
const OUT_PATH = path.join(__dirname, '../../out/AppContainer')
const OUT_ELECTRON_PATH = path.join(OUT_PATH, 'electron')

function isFileNeeded(name) {
  return !/.*\.(?:(pdb)|(ilk)|(exp)|(lib))$/ig.test(name)
}

function isDirNeeded(name) {
  return name === 'resources' || name === 'locales'
}

async function main() {
  if (!fs.existsSync(DEBUG_PATH)) {
    console.log(`Cannot find ${DEBUG_PATH}`)
    return
  }

  fs.emptyDirSync(OUT_PATH)
  fs.emptyDirSync(OUT_ELECTRON_PATH)

  const dirContents = fs.readdirSync(DEBUG_PATH)

  // Copy things over
  for (const item of dirContents) {
    const source = path.join(DEBUG_PATH, item)
    const destination = path.join(OUT_ELECTRON_PATH, item)
    const itemStat = fs.statSync(source)
    const isNeededFile = itemStat.isFile() && isFileNeeded(item)
    const isNeededDir = !isNeededFile && isDirNeeded(item)

    if (isNeededFile || isNeededDir) {
      console.log(`Copying ${item}`)

      await fs.copy(source, destination)
    }
  }
}

main()
