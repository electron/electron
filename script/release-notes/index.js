#!/usr/bin/env node

const { GitProcess } = require('dugite')
const Entities = require('html-entities').AllHtmlEntities
const fetch = require('node-fetch')
const fs = require('fs')
const GitHub = require('github')
const minimist = require('minimist')
const path = require('path')

const CACHE_DIR = path.resolve(__dirname, '.cache')
const entities = new Entities()
const github = new GitHub()
const gitDir = path.resolve(__dirname, '..', '..')
github.authenticate({ type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN })

const semanticMap = new Map()
for (const line of fs.readFileSync(path.resolve(__dirname, 'legacy-pr-semantic-map.csv'), 'utf8').split('\n')) {
  if (!line) continue
  const bits = line.split(',')
  if (bits.length !== 2) continue
  semanticMap.set(bits[0], bits[1])
}

const memLastKnownRelease = new Map()

const getLastKnownReleaseOnBranch = async (branchName) => {
  if (memLastKnownRelease.has(branchName)) {
    return memLastKnownRelease.get(branchName)
  }

  const gitArgs = ['describe', '--abbrev=0', branchName]
  const response = await GitProcess.exec(gitArgs, gitDir)
  if (response.exitCode !== 0) {
    throw GitProcess.parseError(response.stderr)
  }

  const tag = response.stdout.trim()
  memLastKnownRelease.set(branchName, tag)
  return tag
}

const commitBeforeTag = async (commit, tag) => {
  const gitArgs = ['tag', '--contains', commit]
  const tagDetails = await GitProcess.exec(gitArgs, gitDir)
  if (tagDetails.exitCode === 0) {
    return tagDetails.stdout.split('\n').includes(tag)
  }
  throw GitProcess.parseError(tagDetails.stderr)
}

const getPreviousPoint = async (point) => {
  point = point || 'HEAD'
  const gitArgs = ['describe', '--abbrev=0', `${point}^`]
  const response = await GitProcess.exec(gitArgs, gitDir)
  if (response.exitCode === 0) {
    return response.stdout.trim()
  } else {
    console.log(`unable to find closest tag for ${point}; assuming new branch from master`)
    return 'master'
  }
}

const parseCommandLine = async () => {
  let help
  const opts = minimist(process.argv.slice(2), {
    string: [ 'from', 'to' ],
    alias: { 'to': [ 'version', 'new' ], 'from': [ 'baseline', 'old' ] },
    default: { 'to': 'HEAD' },
    unknown: arg => { help = true }
  })
  if (help || opts.help) {
    console.log('Usage: script/release-notes/index.js [--version newVersion, default: HEAD]')
    process.exit(0)
  }
  if (!opts.from) opts.from = await getPreviousPoint(opts.to)
  return opts
}

const getCommitsBetween = async (point1, point2) => {
  const gitArgs = ['rev-list', `${point1}..${point2}`]
  const commitsDetails = await GitProcess.exec(gitArgs, gitDir)
  if (commitsDetails.exitCode !== 0) {
    throw GitProcess.parseError(commitsDetails.stderr)
  }
  return commitsDetails.stdout.trim().split('\n')
}

const TITLE_PREFIX = 'Merged Pull Request: '

// try to get the commit details from the local commit
const getCommitDetailsFromLocal = async (id) => {
  const gitArgs = ['show', '--no-patch', '--format=%s', id]
  const response = await GitProcess.exec(gitArgs, gitDir)
  if (response.exitCode === 0) {
    const body = response.stdout.trim()
    const match = body.match(/^(.*)\s\(#(\d+)\)$/)
    if (match && match.length >= 3) {
      const prTitle = match[1].trim()
      const mergedFrom = parseInt(match[2])
      if (prTitle && mergedFrom) {
        return { prTitle, mergedFrom }
      }
    }
  }
  return null
}

const getCommitDetailsFromGitHub = async (commitHash) => {
  const commitInfo = await (await fetch(`https://github.com/electron/electron/branch_commits/${commitHash}`)).text()
  const bits = commitInfo.split('</a>)')[0].split('>')
  const prIdent = bits[bits.length - 1].trim()
  if (!prIdent || commitInfo.indexOf('href="/electron/electron/pull') === -1) {
    console.warn(`WARNING: Could not track commit "${commitHash}" to a pull request, it may have been committed directly to the branch`)
    return null
  }
  const title = commitInfo.split('title="')[1].split('"')[0]
  if (!title.startsWith(TITLE_PREFIX)) {
    console.warn(`WARNING: Unknown PR title for commit "${commitHash}" in PR "${prIdent}"`)
    return null
  }
  return {
    mergedFrom: parseInt(prIdent.replace('#', '')),
    prTitle: entities.decode(title.substr(TITLE_PREFIX.length))
  }
}

const getCommitDetails = async (commitHash) => {
  let details = await getCommitDetailsFromLocal(commitHash)
  if (!details) details = await getCommitDetailsFromGitHub(commitHash)
  return details
}

const doWork = async (items, fn, concurrent = 5) => {
  const results = []
  const toUse = [].concat(items)
  let i = 1
  const doBit = async () => {
    if (toUse.length === 0) return
    console.log(`Running ${i}/${items.length}`)
    i += 1

    const item = toUse.pop()
    const index = toUse.length
    results[index] = await fn(item)
    await doBit()
  }
  const bits = []
  for (let i = 0; i < concurrent; i += 1) {
    bits.push(doBit())
  }
  await Promise.all(bits)
  return results
}

const notes = new Map()

const NoteType = {
  FIX: 'fix',
  FEATURE: 'feature',
  BREAKING_CHANGE: 'breaking-change',
  DOCUMENTATION: 'doc',
  OTHER: 'other',
  UNKNOWN: 'unknown'
}

class Note {
  constructor (trueTitle, prNumber, ignoreIfInVersion) {
    console.log({ trueTitle, prNumber, ignoreIfInVersion })
    // Self bindings
    this.guessType = this.guessType.bind(this)
    this.fetchPrInfo = this.fetchPrInfo.bind(this)
    this._getPr = this._getPr.bind(this)

    if (!trueTitle.trim()) console.error(prNumber)

    this._ignoreIfInVersion = ignoreIfInVersion
    this.reverted = false
    if (notes.has(trueTitle)) {
      console.warn(`Duplicate PR trueTitle: "${trueTitle}", "${prNumber}" this might cause weird reversions (this would be RARE)`)
    }

    // Memoize
    notes.set(trueTitle, this)

    this.originalTitle = trueTitle
    this.title = trueTitle
    this.prNumber = prNumber
    this.stripColon = true
    if (this.guessType() !== NoteType.UNKNOWN && this.stripColon) {
      this.title = trueTitle.split(':').slice(1).join(':').trim()
    }
  }

  guessType () {
    if (this.originalTitle.startsWith('fix:') ||
        this.originalTitle.startsWith('Fix:')) return NoteType.FIX
    if (this.originalTitle.startsWith('feat:')) return NoteType.FEATURE
    if (this.originalTitle.startsWith('spec:') ||
        this.originalTitle.startsWith('build:') ||
        this.originalTitle.startsWith('test:') ||
        this.originalTitle.startsWith('chore:') ||
        this.originalTitle.startsWith('deps:') ||
        this.originalTitle.startsWith('refactor:') ||
        this.originalTitle.startsWith('tools:') ||
        this.originalTitle.startsWith('vendor:') ||
        this.originalTitle.startsWith('perf:') ||
        this.originalTitle.startsWith('style:') ||
        this.originalTitle.startsWith('ci')) return NoteType.OTHER
    if (this.originalTitle.startsWith('doc:') ||
        this.originalTitle.startsWith('docs:')) return NoteType.DOCUMENTATION

    this.stripColon = false

    if (this.pr && this.pr.data.labels.find(label => label.name === 'semver/breaking-change')) {
      return NoteType.BREAKING_CHANGE
    }
    // FIXME: Backported features will not be picked up by this
    if (this.pr && this.pr.data.labels.find(label => label.name === 'semver/nonbreaking-feature')) {
      return NoteType.FEATURE
    }

    const n = this.prNumber
    if (semanticMap.has(n)) {
      switch (semanticMap.get(n)) {
        case 'feat':
          return NoteType.FEATURE
        case 'fix':
          return NoteType.FIX
        case 'breaking-change':
          return NoteType.BREAKING_CHANGE
        case 'doc':
          return NoteType.DOCUMENTATION
        case 'build':
        case 'vendor':
        case 'refactor':
        case 'spec':
          return NoteType.OTHER
        default:
          throw new Error(`Unknown semantic mapping: ${semanticMap.get(n)}`)
      }
    }
    return NoteType.UNKNOWN
  }

  async _getPr (n) {
    const cachePath = path.resolve(CACHE_DIR, n.toString())
    if (fs.existsSync(cachePath)) {
      return JSON.parse(fs.readFileSync(cachePath, 'utf8'))
    } else {
      try {
        const pr = await github.pullRequests.get({
          number: n,
          owner: 'electron',
          repo: 'electron'
        })
        fs.writeFileSync(cachePath, JSON.stringify({ data: pr.data }))
        return pr
      } catch (err) {
        console.info('#### FAILED:', `#${n}`)
        throw err
      }
    }
  }

  _getReleaseNoteFromPR (pr) {
    // see if user provided a release note in the PR body
    const NOTE_PREFIX = 'Notes: '
    const note = pr.data.body
      .split('\n')
      .map(x => x.trim())
      .find(x => x.startsWith(NOTE_PREFIX))
      .slice(NOTE_PREFIX.length)
      .trim()

    if (note) {
      return note.match(/^[Nn]o[ _-][Nn]otes\.?$/, '') ? null : note
    }

    // fallback: use the title
    if (pr.data.title) {
      return pr.data.title.trim()
    }

    return null
  }

  async fetchPrInfo () {
    console.log(`fetchPrInfo ${this.prNumber}`)
    if (this.pr) return
    const n = this.prNumber
    this.pr = await this._getPr(n)
    this.note = this._getReleaseNoteFromPR(this.pr)

    if (this.pr.data.labels.some(label => label.name === `merged/${this._ignoreIfInVersion.replace('origin/', '')}`)) {
      console.log('this is probably a backport')
      // This means we probably backported this PR, let's try figure out what
      // the corresponding backport PR would be by searching through comments
      // for trop
      let comments
      const cacheCommentsPath = path.resolve(CACHE_DIR, `${n}-comments`)
      if (fs.existsSync(cacheCommentsPath)) {
        comments = JSON.parse(fs.readFileSync(cacheCommentsPath, 'utf8'))
      } else {
        comments = await github.issues.getComments({
          number: n,
          owner: 'electron',
          repo: 'electron',
          per_page: 100
        })
        fs.writeFileSync(cacheCommentsPath, JSON.stringify({ data: comments.data }))
      }

      const tropComment = comments.data.find(
        c => (
          new RegExp(`We have automatically backported this PR to "${this._ignoreIfInVersion.replace('origin/', '')}", please check out #[0-9]+`)
        ).test(c.body)
      )

      if (tropComment) {
        const commentBits = tropComment.body.split('#')
        const tropPrNumber = commentBits[commentBits.length - 1]

        const tropPr = await this._getPr(tropPrNumber)
        if (tropPr.data.merged && tropPr.data.merge_commit_sha) {
          if (await commitBeforeTag(tropPr.data.merge_commit_sha, await getLastKnownReleaseOnBranch(this._ignoreIfInVersion))) {
            this.reverted = true
            console.log('PR', this.prNumber, 'was backported to a previous version, ignoring from notes')
          }
        }
      }
    }
  }
}

Note.findByTrueTitle = (trueTitle) => notes.get(trueTitle)

class ReleaseNotes {
  constructor (ignoreIfInVersion) {
    this._ignoreIfInVersion = ignoreIfInVersion
    this._handledPrs = new Set()
    this._revertedPrs = new Set()
    this.other = []
    this.docs = []
    this.fixes = []
    this.features = []
    this.breakingChanges = []
    this.unknown = []
  }

  async parseCommits (commitHashes) {
    await doWork(commitHashes, async (commit) => {
      const info = await getCommitDetails(commit)
      if (!info) return
      // Only handle each PR once
      if (this._handledPrs.has(info.mergedFrom)) return
      this._handledPrs.add(info.mergedFrom)

      // Strip the trop backport prefix
      const trueTitle = info.prTitle.replace(/^Backport \([0-9]+-[0-9]+-x\) - /, '')
      if (this._revertedPrs.has(trueTitle)) return

      // Handle PRs that revert other PRs
      if (trueTitle.startsWith('Revert "')) {
        const revertedTrueTitle = trueTitle.substr(8, trueTitle.length - 9)
        this._revertedPrs.add(revertedTrueTitle)
        const existingNote = Note.findByTrueTitle(revertedTrueTitle)
        if (existingNote) {
          existingNote.reverted = true
        }
        return
      }

      // Add a note for this PR
      const note = new Note(trueTitle, info.mergedFrom, this._ignoreIfInVersion)
      try {
        await note.fetchPrInfo()
      } catch (err) {
        console.error(commit, info)
        throw err
      }
      switch (note.guessType()) {
        case NoteType.FIX:
          this.fixes.push(note)
          break
        case NoteType.FEATURE:
          this.features.push(note)
          break
        case NoteType.BREAKING_CHANGE:
          this.breakingChanges.push(note)
          break
        case NoteType.OTHER:
          this.other.push(note)
          break
        case NoteType.DOCUMENTATION:
          this.docs.push(note)
          break
        case NoteType.UNKNOWN:
        default:
          this.unknown.push(note)
          break
      }
    }, 20)
  }

  renderNote (note) {
    // clean up the note
    let s = note.note || note.title
    s = s.trim()
    if (s.length !== 0) {
      s = s[0].toUpperCase() + s.substr(1)
      if (!s.endsWith('.')) s = s + '.'
    }

    return `${s} (#${note.prNumber})`
  }

  list (notes, emptyMessage) {
    if (notes.length === 0) {
      return emptyMessage || '_None._'
    }
    return notes
      .filter(note => !note.reverted)
      .map((note) => `* ${this.renderNote(note)}`)
      .sort((a, b) => a.toLowerCase().localeCompare(b.toLowerCase()))
      .join('\n')
  }

  render () {
    return `
# Release Notes

## Breaking Changes

${this.list(this.breakingChanges, '_No breaking changes._')}

## Features

${this.list(this.features, '_No new features._')}

## Fixes

${this.list(this.fixes)}

## Other Changes (E.g. Internal refactors or build system updates)

${this.list(this.other)}

## Documentation Updates

Some documentation updates, fixes and reworks: ${
  this.docs.length === 0
    ? '_None in this release_'
    : this.docs.sort((a, b) => a.prNumber.localeCompare(b.prNumber)).map(note => note.prNumber).join(', ')
}
${this.unknown.filter(n => !n.reverted).length > 0
    ? `## Unknown (fix these before publishing release)

${this.list(this.unknown)}
` : ''}`
  }
}

async function main () {
  if (!fs.existsSync(CACHE_DIR)) {
    fs.mkdirSync(CACHE_DIR)
  }

  const opts = await parseCommandLine()
  console.log(opts)
  const commits = await getCommitsBetween(opts.from, opts.to)
  console.log(commits)

  const notes = new ReleaseNotes(opts.from)
  await notes.parseCommits(commits)
  console.log(notes.render())

  const badNotes = notes.unknown.filter(n => !n.reverted).length
  if (badNotes > 0) {
    throw new Error(`You have ${badNotes.length} unknown release notes, please fix them before releasing`)
  }
}

if (process.mainModule === module) {
  main().catch((err) => {
    console.error('Error Occurred:', err)
    process.exit(1)
  })
}
