try {
  require('runas')
} catch (e) {
  process.exit(1)
}
process.exit(0)
