path = require 'path'

process.__atom_type = 'renderer'
process.resourcesPath = path.resolve process.argv[1], '..', '..', '..'
process.argv.splice 1, 1
