const path = require('path')
const webpack = require('webpack')

const configPath = process.argv[2]
const outPath = path.resolve(process.argv[3])
const config = require(configPath)
config.output = {
  path: path.dirname(outPath),
  filename: path.basename(outPath)
}

webpack(config, (err, stats) => {
  if (err) {
    console.error(err)
    process.exit(1)
  } else if (stats.hasErrors()) {
    console.error(stats.toString('normal'))
    process.exit(1)
  } else {
    process.exit(0)
  }
})
