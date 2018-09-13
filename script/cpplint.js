#!/usr/bin/env node

const { GitProcess } = require('dugite')
const childProcess = require('child_process')
const klaw = require('klaw')
const minimist = require('minimist')
const path = require('path')

const SOURCE_ROOT = path.normalize(path.dirname(__dirname))

function callCpplint (filenames, args) {
  const linter = 'cpplint.py'
  if (args.verbose) console.log([linter, ...filenames].join(' '))
  try {
    childProcess.execFile(linter, filenames, { cwd: SOURCE_ROOT }, error => {
      if (error) {
        for (const line of error.message.split(/[\r\n]+/)) {
          if (line.length && !line.startsWith('Done processing ')) {
            console.warn(line)
          }
        }
        if (error.message.includes('Command failed')) {
          process.exit(1)
        }
      }
    })
  } catch (e) {
    process.exit(1)
  }
}

function parseCommandLine () {
  let help
  const opts = minimist(process.argv.slice(2), {
    boolean: ['help', 'onlyChanged', 'verbose'],
    default: { help: false, onlyChanged: false, verbose: false },
    alias: { c: 'onlyChanged', 'only-changed': 'onlyChanged', h: 'help', v: 'verbose' },
    unknown: arg => { help = true }
  })
  if (help || opts.help) {
    console.log('Usage: cpplint.js [-h|--help] [-c|--only-changed] [-v|--verbose]')
    process.exit(0)
  }
  return opts
}

async function findChangedFiles (top) {
  const result = await GitProcess.exec(['diff', 'HEAD', '--name-only'], top)
  if (result.exitCode !== 0) {
    console.log('Failed to find changed files', GitProcess.parseError(result.stderr))
    process.exit(1)
  }
  const relativePaths = result.stdout.split(/\r\n|\r|\n/g)
  const absolutePaths = relativePaths.map(x => path.join(top, x))
  return new Set(absolutePaths)
}

async function findFiles (top, test) {
  return new Promise((resolve, reject) => {
    const matches = []
    klaw(top)
      .on('end', () => resolve(matches))
      .on('data', item => {
        if (test(item.path)) matches.push(item.path)
      })
  })
}

const blacklist = new Set([
  ['atom', 'browser', 'mac', 'atom_application.h'],
  ['atom', 'browser', 'mac', 'atom_application_delegate.h'],
  ['atom', 'browser', 'resources', 'win', 'resource.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'atom_menu_controller.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'atom_ns_window.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'atom_ns_window_delegate.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'atom_preview_item.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'atom_touch_bar.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'touch_bar_forward_declarations.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'NSColor+Hex.h'],
  ['atom', 'browser', 'ui', 'cocoa', 'NSString+ANSI.h'],
  ['atom', 'common', 'api', 'api_messages.h'],
  ['atom', 'common', 'common_message_generator.cc'],
  ['atom', 'common', 'common_message_generator.h'],
  ['atom', 'common', 'node_includes.h'],
  ['atom', 'node', 'osfhandle.cc'],
  ['brightray', 'browser', 'mac', 'bry_inspectable_web_contents_view.h'],
  ['brightray', 'browser', 'mac', 'event_dispatching_window.h'],
  ['brightray', 'browser', 'mac', 'notification_center_delegate.h'],
  ['brightray', 'browser', 'win', 'notification_presenter_win7.h'],
  ['brightray', 'browser', 'win', 'win32_desktop_notifications', 'common.h'],
  ['brightray', 'browser', 'win', 'win32_desktop_notifications',
    'desktop_notification_controller.cc'],
  ['brightray', 'browser', 'win', 'win32_desktop_notifications',
    'desktop_notification_controller.h'],
  ['brightray', 'browser', 'win', 'win32_desktop_notifications', 'toast.h'],
  ['brightray', 'browser', 'win', 'win32_notification.h']
].map(tokens => path.join(SOURCE_ROOT, ...tokens)))

async function main () {
  const args = parseCommandLine()

  let filenames = []
  const isCCFile = filename => filename.endsWith('.cc') || filename.endsWith('.h')
  const CC_ROOTS = ['atom', 'brightray'].map(x => path.join(SOURCE_ROOT, x))
  for (const root of CC_ROOTS) {
    const files = await findFiles(root, isCCFile)
    filenames.push(...files)
  }

  filenames = filenames.filter(x => !blacklist.has(x))

  if (args.onlyChanged) {
    const whitelist = await findChangedFiles(SOURCE_ROOT)
    filenames = filenames.filter(x => whitelist.has(x))
  }

  if (filenames.length) callCpplint(filenames, args)
}

if (process.mainModule === module) main()
