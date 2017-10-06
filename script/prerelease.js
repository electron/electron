#!/usr/bin/env node

require('colors')
const assert = require('assert')
const GitHub = require('github')
const heads = require('heads')
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
    const drafts = releases
      .filter(release => release.draft) // comment out for testing
      // .filter(release => release.tag_name === 'v1.7.5') // uncomment for testing

    check(drafts.length === 1, 'one draft exists', true)
    const draft = drafts[0]

    check(draft.tag_name === `v${pkg.version}`, `draft release version matches local package.json (v${pkg.version})`)
    check(draft.body.length > 50 && !draft.body.includes('(placeholder)'), 'draft has release notes')

    const requiredAssets = assetsForVersion(draft.tag_name).sort()
    const extantAssets = draft.assets.map(asset => asset.name).sort()

    requiredAssets.forEach(asset => {
      check(extantAssets.includes(asset), asset)
    })

    const s3Urls = s3UrlsForVersion(draft.tag_name)
    heads(s3Urls)
      .then(results => {
        results.forEach((result, i) => {
          check(result === 200, s3Urls[i])
        })

        process.exit(failureCount > 0 ? 1 : 0)
      })
      .catch(err => {
        console.error('Error making HEAD requests for S3 assets')
        console.error(err)
        process.exit(1)
      })
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

function assetsForVersion (version) {
  const patterns = [
    'electron-{{VERSION}}-darwin-x64-dsym.zip',
    'electron-{{VERSION}}-darwin-x64-symbols.zip',
    'electron-{{VERSION}}-darwin-x64.zip',
    'electron-{{VERSION}}-linux-arm-symbols.zip',
    'electron-{{VERSION}}-linux-arm.zip',
    'electron-{{VERSION}}-linux-arm64-symbols.zip',
    'electron-{{VERSION}}-linux-arm64.zip',
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
    'ffmpeg-{{VERSION}}-linux-arm64.zip',
    'ffmpeg-{{VERSION}}-linux-armv7l.zip',
    'ffmpeg-{{VERSION}}-linux-ia32.zip',
    'ffmpeg-{{VERSION}}-linux-x64.zip',
    'ffmpeg-{{VERSION}}-mas-x64.zip',
    'ffmpeg-{{VERSION}}-win32-ia32.zip',
    'ffmpeg-{{VERSION}}-win32-x64.zip'
  ]
  return patterns.map(pattern => pattern.replace(/{{VERSION}}/g, version))
}

function s3UrlsForVersion (version) {
  const bucket = 'https://gh-contractor-zcbenz.s3.amazonaws.com/'
  const patterns = [
    'atom-shell/dist/{{VERSION}}/iojs-{{VERSION}}-headers.tar.gz',
    'atom-shell/dist/{{VERSION}}/iojs-{{VERSION}}.tar.gz',
    'atom-shell/dist/{{VERSION}}/node-{{VERSION}}.tar.gz',
    'atom-shell/dist/{{VERSION}}/node.lib',
    'atom-shell/dist/{{VERSION}}/win-x64/iojs.lib',
    'atom-shell/dist/{{VERSION}}/win-x86/iojs.lib',
    'atom-shell/dist/{{VERSION}}/x64/node.lib',
    'atom-shell/dist/index.json'
  ]
  return patterns.map(pattern => bucket + pattern.replace(/{{VERSION}}/g, version))
}
