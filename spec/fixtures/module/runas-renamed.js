try {
  require('runas')
} catch (e) {
  console.log('bad')
  process.exit(1)
}
console.log('good')
process.exit(0)
  