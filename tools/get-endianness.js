var os = require('os')
if (os.endianness) {
  console.log(require('os').endianness() === 'BE' ? 'big' : 'little')
} else {  // Your Node is rather old, but I don't care.
  console.log('little')
}
