const { GitProcess } = require('dugite')
const path = require('path')

const SOURCE_ROOT = path.normalize(path.dirname(__dirname))

async function findChangedFiles () {
  if (process.argv.length === 3) {
    const filesChanged = await GitProcess.exec(['diff-tree', '--no-commit-id', '--name-only', '-r', process.argv[2]], SOURCE_ROOT)
    if (filesChanged.exitCode === 0) {
      const fileList = filesChanged.stdout.split('\n')
      const nonDocChange = fileList.find((filePath) => {
        const fileDirs = filePath.split(path.sep)
        if (fileDirs[0] !== 'docs') {
          return true
        }
      })
      if (nonDocChange) {
        console.log('false')
      } else {
        console.log('true')
      }
    } else {
      console.log('Error getting list of files changed: ', filesChanged)
      process.exit(-1)
    }
  } else {
    console.log(`Check if only the docs were changed for a commit.
    Usage: doc-only-change.js COMMIT_SHA`)
    process.exit(-1)
  }
}

findChangedFiles()
