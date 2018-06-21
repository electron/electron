const { GitProcess } = require('dugite')
const Entities = require('html-entities').AllHtmlEntities
const fetch = require('node-fetch')
const fs = require('fs')
const GitHub = require('github')
const path = require('path')
const semver = require('semver')

const CACHE_DIR = path.resolve(__dirname, '.cache')
// Fill this with tags to ignore if you are generating release notes for older
// versions
//
// E.g. ['v3.0.0-beta.1'] to generate the release notes for 3.0.0-beta.1 :) from
//      the current 3-0-x branch
const EXCLUDE_TAGS = []

const entities = new Entities()
const github = new GitHub()
const gitDir = path.resolve(__dirname, '..', '..')
github.authenticate({ type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN })
let currentBranch

const semanticMap = new Map()
for (const line of fs.readFileSync(path.resolve(__dirname, 'legacy-pr-semantic-map.csv'), 'utf8').split('\n')) {
  if (!line) continue
  const bits = line.split(',')
  if (bits.length !== 2) continue
  semanticMap.set(bits[0], bits[1])
}

const getCurrentBranch = async () => {
  if (currentBranch) return currentBranch
  const gitArgs = ['rev-parse', '--abbrev-ref', 'HEAD']
  const branchDetails = await GitProcess.exec(gitArgs, gitDir)
  if (branchDetails.exitCode === 0) {
    currentBranch = branchDetails.stdout.trim()
    return currentBranch
  }
  throw GitProcess.parseError(branchDetails.stderr)
}

const getBranchOffPoint = async (branchName) => {
  const gitArgs = ['merge-base', branchName, 'master']
  const commitDetails = await GitProcess.exec(gitArgs, gitDir)
  if (commitDetails.exitCode === 0) {
    return commitDetails.stdout.trim()
  }
  throw GitProcess.parseError(commitDetails.stderr)
}

const getTagsOnBranch = async (branchName) => {
  const gitArgs = ['tag', '--merged', branchName]
  const tagDetails = await GitProcess.exec(gitArgs, gitDir)
  if (tagDetails.exitCode === 0) {
    return tagDetails.stdout.trim().split('\n').filter(tag => !EXCLUDE_TAGS.includes(tag))
  }
  throw GitProcess.parseError(tagDetails.stderr)
}

const memLastKnownRelease = new Map()

const getLastKnownReleaseOnBranch = async (branchName) => {
  if (memLastKnownRelease.has(branchName)) {
    return memLastKnownRelease.get(branchName)
  }
  const tags = await getTagsOnBranch(branchName)
  if (!tags.length) {
    throw new Error(`Branch ${branchName} has no tags, we have no idea what the last release was`)
  }
  const branchOffPointTags = await getTagsOnBranch(await getBranchOffPoint(branchName))
  if (branchOffPointTags.length >= tags.length) {
    // No release on this branch
    return null
  }
  memLastKnownRelease.set(branchName, tags[tags.length - 1])
  // Latest tag is the latest release
  return tags[tags.length - 1]
}

const getBranches = async () => {
  const gitArgs = ['branch', '--remote']
  const branchDetails = await GitProcess.exec(gitArgs, gitDir)
  if (branchDetails.exitCode === 0) {
    return branchDetails.stdout.trim().split('\n').map(b => b.trim()).filter(branch => branch !== 'origin/HEAD -> origin/master')
  }
  throw GitProcess.parseError(branchDetails.stderr)
}

const semverify = (v) => v.replace(/^origin\//, '').replace('x', '0').replace(/-/g, '.')

const getLastReleaseBranch = async () => {
  const current = await getCurrentBranch()
  const allBranches = await getBranches()
  const releaseBranches = allBranches
    .filter(branch => /^origin\/[0-9]+-[0-9]+-x$/.test(branch))
    .filter(branch => branch !== current && branch !== `origin/${current}`)
  let latest = null
  for (const b of releaseBranches) {
    if (latest === null) latest = b
    if (semver.gt(semverify(b), semverify(latest))) {
      latest = b
    }
  }
  return latest
}

const commitBeforeTag = async (commit, tag) => {
  const gitArgs = ['tag', '--contains', commit]
  const tagDetails = await GitProcess.exec(gitArgs, gitDir)
  if (tagDetails.exitCode === 0) {
    return tagDetails.stdout.split('\n').includes(tag)
  }
  throw GitProcess.parseError(tagDetails.stderr)
}

const getCommitsMergedIntoCurrentBranchSincePoint = async (point) => {
  return getCommitsBetween(point, 'HEAD')
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

const getCommitDetails = async (commitHash) => {
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
    mergedFrom: prIdent,
    prTitle: entities.decode(title.substr(TITLE_PREFIX.length))
  }
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

    const n = this.prNumber.replace('#', '')
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
    const cachePath = path.resolve(CACHE_DIR, n)
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

  async fetchPrInfo () {
    if (this.pr) return
    const n = this.prNumber.replace('#', '')
    this.pr = await this._getPr(n)
    if (this.pr.data.labels.find(label => label.name === `merged/${this._ignoreIfInVersion.replace('origin/', '')}`)) {
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

  list (notes) {
    if (notes.length === 0) {
      return '_There are no items in this section this release_'
    }
    return notes
      .filter(note => !note.reverted)
      .sort((a, b) => a.title.toLowerCase().localeCompare(b.title.toLowerCase()))
      .map((note) => `* ${note.title.trim()} ${note.prNumber}`).join('\n')
  }

  render () {
    return `
# Release Notes

## Breaking Changes

${this.list(this.breakingChanges)}

## Features

${this.list(this.features)}

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
  const lastReleaseBranch = await getLastReleaseBranch()

  const notes = new ReleaseNotes(lastReleaseBranch)
  const lastKnownReleaseInCurrentStream = await getLastKnownReleaseOnBranch(await getCurrentBranch())
  const currentBranchOff = await getBranchOffPoint(await getCurrentBranch())

  const commits = await getCommitsMergedIntoCurrentBranchSincePoint(
    lastKnownReleaseInCurrentStream || currentBranchOff
  )

  if (!lastKnownReleaseInCurrentStream) {
    // This means we are the first release in our stream
    // FIXME: This will not work for minor releases!!!!

    const lastReleaseBranch = await getLastReleaseBranch()
    const lastBranchOff = await getBranchOffPoint(lastReleaseBranch)
    commits.push(...await getCommitsBetween(lastBranchOff, currentBranchOff))
  }

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
