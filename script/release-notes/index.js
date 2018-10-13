#!/usr/bin/env node

const { GitProcess } = require('dugite')
const fetch = require('node-fetch')
const fs = require('fs')
const GitHub = require('github')
const minimist = require('minimist')
const path = require('path')

const CACHE_DIR = path.resolve(__dirname, '.cache')
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

const getCommitDetails = async (hash, owner = 'electron', repo = 'electron') => {
  const commit = await getCommit(hash, owner, repo)
  if (!commit || !commit.commit || !commit.commit.message) { return null }

  const [ title ] = commit.commit.message.split('\n', 2).map(x => x.trim())
  const titleRe = /^(.*)\s\(#(\d+)\)$/
  const match = title.match(titleRe)
  if (match) {
    return {
      prEmail: commit.commit.author.email,
      prTitle: match[1],
      mergedFrom: parseInt(match[2])
    }
  }

  return null
}

const getCommit = async (hash, owner = 'electron', repo = 'electron') => {
  const cachePath = path.resolve(CACHE_DIR, `${owner}-${repo}-commit-${hash}`)
  if (fs.existsSync(cachePath)) {
    return JSON.parse(fs.readFileSync(cachePath, 'utf8'))
  }
  const url = `https://api.github.com/repos/${owner}/${repo}/commits/${hash}`
  const response = await (await fetch(url)).json()
  fs.writeFileSync(cachePath, JSON.stringify(response, null, 2))
  return response
}

const getPullRequest = async (number, owner = 'electron', repo = 'electron') => {
  const cachePath = path.resolve(CACHE_DIR, `${owner}-${repo}-pull-${number}`)
  console.log({ cachePath })
  if (fs.existsSync(cachePath)) {
    return JSON.parse(fs.readFileSync(cachePath, 'utf8'))
  }
  const response = await github.pullRequests.get({ number, owner, repo })
  fs.writeFileSync(cachePath, JSON.stringify({ data: response.data }))
  return response
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
  constructor (trueTitle, prNumber, ignoreIfInVersion, owner, repo) {
    console.log({ trueTitle, prNumber, ignoreIfInVersion })
    // Self bindings
    this.guessType = this.guessType.bind(this)
    this.fetchPrInfo = this.fetchPrInfo.bind(this)

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
    this.owner = owner
    this.repo = repo
    this.stripColon = true
    if (this.guessType() !== NoteType.UNKNOWN && this.stripColon) {
      this.title = trueTitle.split(':').slice(1).join(':').trim()
    }
  }

  guessType () {
    console.log({ originalTitle: this.originalTitle })
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

  _getTextFromPr (pr) {
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
    if (pr.title) {
      return pr.title.trim()
    }

    return null
  }

  async fetchPrInfo () {
    if (this.pr) return
    const n = this.prNumber
    this.pr = await getPullRequest(n, this.owner, this.repo)
    this.text = this._getTextFromPr(this.pr)
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

        const tropPr = await getPullRequest(tropPrNumber, this.owner, this.repo)
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

  _createNoteFromDetails (details, owner = 'electron', repo = 'electron') {
    // Strip the trop backport prefix
    const trueTitle = details.prTitle.replace(/^Backport \([0-9]+-[0-9]+-x\) - /, '')
    if (this._revertedPrs.has(trueTitle)) return null

    // Handle PRs that revert other PRs
    if (trueTitle.startsWith('Revert "')) {
      const revertedTrueTitle = trueTitle.substr(8, trueTitle.length - 9)
      this._revertedPrs.add(revertedTrueTitle)
      const existingNote = Note.findByTrueTitle(revertedTrueTitle)
      if (existingNote) {
        existingNote.reverted = true
      }
      return null
    }

    return new Note(trueTitle, details.mergedFrom, this._ignoreIfInVersion, owner, repo)
  }

  async _addNote (note) {
    if (!note) return
    try {
      await note.fetchPrInfo()
    } catch (err) {
      console.error(note, err)
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
  }

  async parseCommits (commitHashes) {
    await doWork(commitHashes, async (commit) => {
      const details = await getCommitDetails(commit)
      if (!details) return

      // Only handle each PR once
      if (this._handledPrs.has(details.mergedFrom)) return
      this._handledPrs.add(details.mergedFrom)

      // handle roller-bot PRs
      if (details.prEmail.includes('roller-bot')) {
        const pr = await getPullRequest(details.mergedFrom)
        if (!pr || !pr.data || !pr.data.body || !pr.data.body.includes('Changes since the last roll:')) { return }
        // https://github.com/electron/roller/blob/d0bbe852f2ed91e1f3098445567f54c8ffcc867b/src/pr.ts#L33
        // `* [\`${commit.sha.substr(0, 8)}\`](${COMMIT_URL_BASE}/${commit.sha}) ${commit.message}`).join('\n')}
        const re = /https:\/\/github.com\/([^/]+)\/([^/]+)\/commit\/+([0-9A-Fa-f]+)/
        for (const change of pr.data.body.split('\n').map(x => x.trim().match(re)).filter(x => x && x.length >= 3)) {
          const [ , owner, repo, hash ] = change
          const rollDetails = await getCommitDetails(hash, owner, repo)
          await this._addNote(this._createNoteFromDetails(rollDetails, owner, repo))
        }
        return
      }

      await this._addNote(this._createNoteFromDetails(details))
    }, 20)
  }

  renderNote (note) {
    // clean up the note
    let s = note.text || note.title
    s = s.trim()
    if (s.length !== 0) {
      s = s[0].toUpperCase() + s.substr(1)
      if (!s.endsWith('.')) s = s + '.'
    }

    if (note.owner === 'electron' && note.repo === 'electron') { return `${s} (#${note.prNumber})` }

    return `${s} ([#${note.prNumber}](https://github.com/${note.owner}/${note.repo}/pull/${note.prNumber}))`
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
