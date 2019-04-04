const cp = require('child_process')
const fs = require('fs-extra')
const os = require('os')
const path = require('path')

const rootPath = path.resolve(__dirname, '..')
const gniPath = path.resolve(__dirname, '../filenames.auto.gni')

const allDocs = fs.readdirSync(path.resolve(__dirname, '../docs/api'))
  .map(doc => `docs/api/${doc}`)
  .concat(
    fs.readdirSync(path.resolve(__dirname, '../docs/api/structures'))
      .map(doc => `docs/api/structures/${doc}`)
  )

const main = async () => {
  const browserifyTargets = [
    {
      name: 'sandbox_browserify_deps',
      entry: 'lib/sandboxed_renderer/init.js'
    },
    {
      name: 'isolated_browserify_deps',
      entry: 'lib/isolated_renderer/init.js'
    },
    {
      name: 'context_script_browserify_deps',
      entry: 'lib/content_script/init.js'
    }
  ]

  await Promise.all(browserifyTargets.map(async browserifyTarget => {
    const tmpDir = await fs.mkdtemp(path.resolve(os.tmpdir(), 'electron-filenames-'))
    const child = cp.spawn('node', [
      'node_modules/browserify/bin/cmd.js',
      browserifyTarget.entry,
      ...(browserifyTarget.name === 'sandbox_browserify_deps' ? [
        '-r',
        './lib/sandboxed_renderer/api/exports/electron.js:electron'
      ] : []),
      '-t',
      'aliasify',
      '-p',
      '[',
      'tsify',
      '-p',
      'tsconfig.electron.json',
      ']',
      '-o',
      path.resolve(tmpDir, 'out.js'),
      '--list'
    ], {
      cwd: path.resolve(__dirname, '..')
    })
    let output = ''
    child.stdout.on('data', chunk => {
      output += chunk.toString()
    })
    await new Promise((resolve, reject) => child.on('exit', (code) => {
      if (code !== 0) return reject(new Error(`Failed to list browserify dependencies for entry: ${browserifyTarget.name}`))

      resolve()
    }))

    browserifyTarget.dependencies = output
      .split('\n')
      // Remove whitespace
      .map(line => line.trim())
      // Ignore empty lines
      .filter(line => line)
      // Get the relative path
      .map(line => path.relative(rootPath, line))
      // Only care about files in //electron
      .filter(line => !line.startsWith('..'))
      // Only care about our own files
      .filter(line => !line.startsWith('node_modules'))
      // Make the generated list easier to read
      .sort()
      // All browserify commands depend on the tsconfig  and package json files
      .concat(['tsconfig.json', 'tsconfig.electron.json', 'package.json'])
    await fs.remove(tmpDir)
  }))

  fs.writeFileSync(
    gniPath,
    `# THIS FILE IS AUTO-GENERATED, PLEASE DO NOT EDIT BY HAND
auto_filenames = {
  api_docs = [
${allDocs.map(doc => `    "${doc}",`).join('\n')}
  ]

${browserifyTargets.map(target => `  ${target.name} = [
${target.dependencies.map(dep => `    "${dep}",`).join('\n')}
  ]`).join('\n\n')}
}
`)
}

if (process.mainModule === module) {
  main().catch((err) => {
    console.error(err)
    process.exit(1)
  })
}
