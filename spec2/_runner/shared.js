const topLevelSuites = []
const suiteChain = []
let _currentFile

module.exports = {
  getCurrentFile() {
    return _currentFile
  },
  setCurrentFile(f) {
    _currentFile = f
  },
  getTopLevelSuites() {
    return topLevelSuites
  },
  getSuiteChain() {
    return suiteChain
  }
}
