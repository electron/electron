#!/usr/bin/env node

require('colors')
const assert = require('assert')
const GitHub = require('github')
const pkg = require('../package.json')
const pass = '\u2713'.green
const fail = '\u2717'.red
let failureCount = 0

assert(process.env.ELECTRON_GITHUB_TOKEN, 'ELECTRON_GITHUB_TOKEN not found in environment')

const github = new GitHub()
github.authenticate({type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN})
github.repos.getReleases({owner: 'electron', repo: 'electron'})
  .then(res => {
    const releases = res.data
    const drafts = releases.filter(release => release.draft)

    check(drafts.length === 1, 'one draft exists', true)
    const draft = drafts[0]

    check(draft.tag_name === `v${pkg.version}`, `draft release version matches local package.json (v${pkg.version})`)
    check(draft.prerelease, 'draft is a prerelease')
    check(draft.body.length > 50 && !draft.body.includes('(placeholder)'), 'draft has release notes')

    const requiredAssets = filenames(draft.tag_name).sort()
    const extantAssets = draft.assets.map(asset => asset.name).sort()

    check(requiredAssets.length === extantAssets.length, 'draft has expected number of assets')
    requiredAssets.forEach(asset => {
      check(extantAssets.includes(asset), asset)
    })

    process.exit(failureCount > 0 ? 1 : 0)
  })

function check (condition, statement, exitIfFail = false) {
  if (condition) {
    console.log(`${pass} ${statement}`)
  } else {
    failureCount++
    console.log(`${fail} ${statement}`)
    if (exitIfFail) process.exit(1)
  }
}

function filenames (version) {
  const patterns = [
    'electron-{{VERSION}}-darwin-x64-dsym.zip',
    'electron-{{VERSION}}-darwin-x64-symbols.zip',
    'electron-{{VERSION}}-darwin-x64.zip',
    'electron-{{VERSION}}-linux-arm-symbols.zip',
    'electron-{{VERSION}}-linux-arm.zip',
    'electron-{{VERSION}}-linux-armv7l-symbols.zip',
    'electron-{{VERSION}}-linux-armv7l.zip',
    'electron-{{VERSION}}-linux-ia32-symbols.zip',
    'electron-{{VERSION}}-linux-ia32.zip',
    'electron-{{VERSION}}-linux-x64-symbols.zip',
    'electron-{{VERSION}}-linux-x64.zip',
    'electron-{{VERSION}}-mas-x64-dsym.zip',
    'electron-{{VERSION}}-mas-x64-symbols.zip',
    'electron-{{VERSION}}-mas-x64.zip',
    'electron-{{VERSION}}-win32-ia32-pdb.zip',
    'electron-{{VERSION}}-win32-ia32-symbols.zip',
    'electron-{{VERSION}}-win32-ia32.zip',
    'electron-{{VERSION}}-win32-x64-pdb.zip',
    'electron-{{VERSION}}-win32-x64-symbols.zip',
    'electron-{{VERSION}}-win32-x64.zip',
    'electron-api.json',
    'electron.d.ts',
    'ffmpeg-{{VERSION}}-darwin-x64.zip',
    'ffmpeg-{{VERSION}}-linux-arm.zip',
    'ffmpeg-{{VERSION}}-linux-armv7l.zip',
    'ffmpeg-{{VERSION}}-linux-ia32.zip',
    'ffmpeg-{{VERSION}}-linux-x64.zip',
    'ffmpeg-{{VERSION}}-mas-x64.zip',
    'ffmpeg-{{VERSION}}-win32-ia32.zip',
    'ffmpeg-{{VERSION}}-win32-x64.zip'
  ]
  return patterns.map(pattern => pattern.replace('{{VERSION}}', version))
}
