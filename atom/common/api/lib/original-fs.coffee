vm = require 'vm'

# Execute the 'fs.js' and pass the 'exports' to it.
source = '(function (exports, require, module, __filename, __dirname) { ' +
         process.binding('natives').originalFs +
         '\n});'
fn = vm.runInThisContext source, { filename: 'fs.js' }
fn exports, require, module
