#!/usr/bin/env node

const { GitProcess } = require('dugite')
const fetch = require('node-fetch')
const fs = require('fs')
const GitHub = require('github')
const path = require('path')

const CACHE_DIR = path.resolve(__dirname, '.cache')
const DEFAULT_OWNER = 'electron'
const NO_NOTES = 'No notes'
const DEFAULT_REPO = 'electron'
const github = new GitHub()
const gitDir = path.resolve(__dirname, '..', '..')
github.authenticate({ type: 'token', token: process.env.ELECTRON_GITHUB_TOKEN })

const breakTypes = new Set(['breaking-change'])
const docTypes = new Set(['doc', 'docs'])
const featTypes = new Set(['feat', 'feature'])
const fixTypes = new Set(['fix'])
const otherTypes = new Set(['spec', 'build', 'test', 'chore', 'deps', 'refactor', 'tools', 'vendor', 'perf', 'style', 'ci'])
const knownTypes = new Set([...breakTypes.keys(), ...docTypes.keys(), ...featTypes.keys(), ...fixTypes.keys(), ...otherTypes.keys()])

const semanticMap = new Map()
for (const line of fs.readFileSync(path.resolve(__dirname, 'legacy-pr-semantic-map.csv'), 'utf8').split('\n')) {
  if (!line) continue
  const bits = line.split(',')
  if (bits.length !== 2) continue
  semanticMap.set(bits[0], bits[1])
}

const runGit = async (args) => {
  // console.log('runGit', args)
  const response = await GitProcess.exec(args, gitDir)
  if (response.exitCode !== 0) throw new Error(response.stderr.trim())
  return response.stdout.trim()
}

const getCommonAncestor = async (point1, point2) => {
  return runGit(['merge-base', point1, point2])
}

const setPullRequest = (o, owner, repo, number) => {
  if (!owner || !repo || !number) throw new Error(JSON.stringify({ owner, repo, number }, null, 2))
  if (!o.originalPr) o.originalPr = o.pr
  o.pr = { owner, repo, number }
  if (!o.originalPr) o.originalPr = o.pr
}

/**
 * Looks for our project's conventions in the commit message:
 *
 * 'semantic: some description' -- sets type, subject
 * 'some description (#99999)' -- sets subject, pr
 * 'Fixes #3333' -- sets issueNumber
 * 'Merge pull request #99999 from ${branchname}' -- sets pr
 * 'This reverts commit ${sha}' -- sets revertHash
 * line starting with 'BREAKING CHANGE' in body -- sets breakingChange
 * 'Backport of #99999' -- sets pr
 */
const parseCommitMessage = (commitMessage, owner, repo, o = {}) => {
  // console.log({ commitMessage })

  // split commitMessage into subject & body
  let subject = commitMessage
  let body = ''
  const pos = subject.indexOf('\n')
  if (pos !== -1) {
    body = subject.slice(pos).trim()
    subject = subject.slice(0, pos).trim()
  }

  if (!o.originalSubject) o.originalSubject = subject

  if (body) o.body = body

  // if the subject ends in ' (#dddd)', treat it as a pull request id
  let match
  if ((match = subject.match(/^(.*)\s\(#(\d+)\)$/))) {
    setPullRequest(o, owner, repo, parseInt(match[2]))
    subject = match[1]
  }

  // if the subject begins with 'word:', treat it as a semantic commit
  if ((match = subject.match(/^(\w+):\s(.*)$/))) {
    o.type = match[1].toLocaleLowerCase()
    subject = match[2]
  }

  // Check for GitHub commit message that indicates a PR
  if ((match = subject.match(/^Merge pull request #(\d+) from (.*)$/))) {
    setPullRequest(o, owner, repo, parseInt(match[1]))
    o.pr.branch = match[2].trim()
  }

  // Check for a trop comment that indicates a PR
  if ((match = commitMessage.match(/\bBackport of #(\d+)\b/))) {
    setPullRequest(o, owner, repo, parseInt(match[1]))
  }

  // https://help.github.com/articles/closing-issues-using-keywords/
  if ((match = subject.match(/\b(?:close|closes|closed|fix|fixes|fixed|resolve|resolves|resolved|for)\s#(\d+)\b/))) {
    o.issueNumber = parseInt(match[1])
    if (!o.type) o.type = 'fix'
  }

  // look for 'fixes' in markdown; e.g. 'Fixes [#8952](https://github.com/electron/electron/issues/8952)'
  if (!o.issueNumber && ((match = commitMessage.match(/Fixes \[#(\d+)\]\(https:\/\/github.com\/(\w+)\/(\w+)\/issues\/(\d+)\)/)))) {
    o.issueNumber = parseInt(match[1])
    if (o.pr && o.pr.number === o.issueNumber) o.pr = null
    if (o.originalPr && o.originalPr.number === o.issueNumber) o.originalPr = null
    if (!o.type) o.type = 'fix'
  }

  // https://www.conventionalcommits.org/en
  if (body.split('\n').map(x => x.trim()).some(x => x.startsWith('BREAKING CHANGE'))) {
    o.type = 'breaking-change'
  }

  // Check for a reversion commit
  if ((match = body.match(/This reverts commit ([a-f0-9]{40})\./))) {
    o.revertHash = match[1]
  }

  // Edge case: manual backport where commit has a link to the backport PR
  if (commitMessage.includes('ackport') &&
      ((match = commitMessage.match(/https:\/\/github\.com\/(\w+)\/(\w+)\/pull\/(\d+)/)))) {
    const [ , owner, repo, number ] = match
    setPullRequest(o, owner, repo, number)
  }

  // Legacy commits: pre-semantic commits
  if (!o.type) {
    const s = commitMessage.toLocaleLowerCase()
    if ((match = s.match(/\bchore\((\w+)\):/))) {
      // example: 'Chore(docs): description'
      o.type = knownTypes.has(match[1]) ? match[1] : 'chore'
    } else if (s.match(/\b(?:fix|fixes|fixed)/)) {
      // example: 'fix a bug'
      o.type = 'fix'
    } else if (s.match(/\[(?:docs|doc)\]/)) {
      // example: '[docs]
      o.type = 'doc'
    }
  }

  o.subject = subject.trim()
  // console.log('parse returning', JSON.stringify(o, null, 2))
  return o
}

/*
 * possible properties:
 * breakingChange, email, hash, issueNumber, originalSubject, parentHashes,
 * pr { owner, repo, number, branch }, revertHash, subject, type
 */
const getLocalCommitDetails = async (point1, point2) => {
  // console.log({ point1, point2 })
  const fieldSep = '||'
  const format = ['%H', '%P', '%aE', '%B'].join(fieldSep)
  const args = ['log', '-z', '--first-parent', `--format=${format}`, `${point1}..${point2}`]
  const commits = (await runGit(args)).split(`\0`).map(x => x.trim())
  const details = []
  for (const commit of commits) {
    if (!commit) continue
    const [ hash, parentHashes, email, commitMessage ] = commit.split(fieldSep, 4).map(x => x.trim())
    details.push(parseCommitMessage(commitMessage, DEFAULT_OWNER, DEFAULT_REPO, {
      email,
      hash,
      parentHashes: parentHashes.split()
    }))
  }
  return details
}

const checkCache = async (name, operation) => {
  const filename = path.resolve(CACHE_DIR, name)
  // console.log({ filename })
  if (fs.existsSync(filename)) {
    return JSON.parse(fs.readFileSync(filename, 'utf8'))
  }
  const response = await operation()
  if (response) {
    fs.writeFileSync(filename, JSON.stringify(response))
  }
  return response
}

const getCommit = async (hash, owner, repo) => {
  const name = `${owner}-${repo}-commit-${hash}`
  return checkCache(name, async () => {
    const url = `https://api.github.com/repos/${owner}/${repo}/commits/${hash}`
    return (await fetch(url)).json()
  })
}

const getPullRequest = async (number, owner, repo) => {
  const name = `${owner}-${repo}-pull-${number}`
  return checkCache(name, async () => {
    try {
      return await github.pullRequests.get({ number, owner, repo })
    } catch (e) {
      // Silently eat 404s.
      // We can get a bad pull number if someone manually lists
      // an issue number in PR number notation, e.g. 'fix: foo (#123)'
      if (e.code !== 404) throw e
    }
  })
}

// see if user provided a release note in the PR body
const getNoteFromPull = pull => {
  const NOTE_PREFIX = 'Notes: '

  if (!pull || !pull.data || !pull.data.body) {
    return null
  }

  let note = pull.data.body
    .split('\n')
    .map(x => x.trim())
    .find(x => x.startsWith(NOTE_PREFIX))

  if (note) {
    const placeholder = '<!-- One-line Change Summary Here-->'
    note = note
      .slice(NOTE_PREFIX.length)
      .replace(placeholder, '')
      .trim()
  }

  if (note) {
    if (note.match(/^[Nn]o[ _-][Nn]otes\.?$/)) {
      return NO_NOTES
    }
    if (note.match(/^[Nn]one\.?$/)) {
      return NO_NOTES
    }
  }

  return note
}

const shouldIgnore = details => {
  // skip bump commits
  if (details.subject.match(/^Bump v\d+\.\d+\.\d+/)) {
    return true
  }

  // skip procedural commits
  if (details.note === NO_NOTES) {
    return true
  }

  return false
}

const getRollerCommits = async (pr) => {
  const commits = []
  if (pr && pr.data && pr.data.user &&
      pr.data.user.login === 'roller-bot' &&
      pr.data.body.includes('Changes since the last roll:')) {
    // https://github.com/electron/roller/blob/d0bbe852f2ed91e1f3098445567f54c8ffcc867b/src/pr.ts#L33
    // `* [\`${commit.sha.substr(0, 8)}\`](${COMMIT_URL_BASE}/${commit.sha}) ${commit.message}`).join('\n')}
    for (const line of pr.data.body.split('\n').map(x => x.trim()).filter(x => !!x)) {
      const match = line.match(/^\*\s\[`[0-9a-f]{8}`\]\(https:\/\/github.com\/(\w+)\/(\w+)\/commit\/+([0-9a-f]{40})\)\s(.*)$/)
      if (!match) continue

      const [ , owner, repo, hash, message ] = match
      const o = parseCommitMessage(message, owner, repo)

      const commitData = await getCommit(hash, owner, repo)
      if (commitData && commitData.commit && commitData.commit.message) {
        parseCommitMessage(commitData.commit.message, owner, repo, {
          email: commitData.commit.author.email,
          hash: commitData.sha,
          ...o
        })
      }

      commits.push(o)
    }
  }

  // console.log('rollerCommits', { commits })
  return commits
}

const getNotes = async (fromRef, toRef) => {
  if (!fs.existsSync(CACHE_DIR)) {
    fs.mkdirSync(CACHE_DIR)
  }

  // get the relevant commits
  const commonAncestor = await getCommonAncestor(fromRef, toRef)
  let commits = await getLocalCommitDetails(commonAncestor, toRef)

  // add old commits to the processed set
  const oldCommits = await getLocalCommitDetails(commonAncestor, fromRef)
  const processedHashes = new Set(oldCommits.map(x => x.hash))
  // add commit + revert commit pairs to the processed set
  commits.forEach(x => {
    if (x.revertHash) {
      processedHashes.add(x.revertHash)
      processedHashes.add(x.hash)
    }
  })
  // remove the pre-processed commits
  commits = commits.filter(x => !processedHashes.has(x.hash))

  // console.log(JSON.stringify(commits, null, 2))

  // process all the commits' pull requests
  const queue = commits
  commits = []
  while (queue.length) {
    const commit = queue.shift()
    if (commit.pr) {
      const pr = await getPullRequest(commit.pr.number, commit.pr.owner, commit.pr.repo)
      if (pr && pr.data) {
        processedHashes.add(commit.hash)

        const note = getNoteFromPull(pr)
        if (note) {
          commit.note = note
        }

        const rollerCommits = await getRollerCommits(pr)
        if (rollerCommits.length) {
          rollerCommits.forEach(x => { x.originalPr = commit.originalPr })
          queue.push(...rollerCommits)
          continue
        }

        if (pr.data && pr.data.title) {
          const number = commit.pr ? commit.pr.number : 0
          parseCommitMessage([pr.data.title, pr.data.body].join('\n'), commit.pr.owner, commit.pr.repo, commit)
          if (number !== commit.pr.number) {
            console.log(`prNumber changed from ${number} to ${commit.pr.number} ... re-running`)
            queue.push(commit)
            continue
          }
        }
      }
    }

    commits.push(commit)
  }

  // remove unininteresting commits
  commits = commits.filter(x => !shouldIgnore(x))
  // console.log(JSON.stringify(commits, null, 2))

  const o = {
    breaks: [],
    docs: [],
    feat: [],
    fix: [],
    other: [],
    unknown: [],
    ref: toRef
  }

  commits.forEach(x => {
    const str = x.type
    if (!str) o.unknown.push(x)
    else if (breakTypes.has(str)) o.breaks.push(x)
    else if (docTypes.has(str)) o.docs.push(x)
    else if (featTypes.has(str)) o.feat.push(x)
    else if (fixTypes.has(str)) o.fix.push(x)
    else if (otherTypes.has(str)) o.other.push(x)
    else o.unknown.push(x)
  })

  // console.log(JSON.stringify(o, null, 2))

  return o
}

/***
****  Render
***/

const renderCommit = commit => {
  // clean up the note
  let note = commit.note || commit.subject
  note = note.trim()
  if (note.length !== 0) {
    note = note[0].toUpperCase() + note.substr(1)
    if (!note.endsWith('.')) note = note + '.'

    for (const key of ['Fix ', 'Fixes ']) {
      if (note.startsWith(key)) note = 'Fixed ' + note.slice(key.length)
    }
  }

  const pr = commit.originalPr
  if (pr) {
    return pr.owner === DEFAULT_OWNER && pr.repo === DEFAULT_REPO
      ? `${note} #${pr.number}`
      : `${note} [#${pr.number}](https://github.com/${pr.owner}/${pr.repo}/pull/${pr.number})`
  }

  // no pull request!
  // someone may have pushed this without a PR
  return `${note} (https://github.com/electron/electron/commit/${commit.hash})`
}

const renderNotes = notes => {
  const rendered = [ `# Release Notes for ${notes.ref}\n\n` ]

  const renderSection = (title, commits) => {
    if (commits.length === 0) return
    const lines = commits.map(x => ` * ${renderCommit(x)}\n`).sort()
    rendered.push(`## ${title}\n\n`, ...lines, '\n')
  }

  renderSection('Breaking Changes', notes.breaks)
  renderSection('Features', notes.feat)
  renderSection('Fixes', notes.fix)
  renderSection('Other Changes', notes.other)

  if (notes.docs.length) {
    const docs = notes.docs.map(x => {
      if (x.pr && x.pr.number) return `#${x.pr.number}`
      return `https://github.com/electron/electron/commit/${x.hash}`
    }).sort()
    rendered.push('## Documentation\n\n', ` * Documentation changes: ${docs.join(', ')}\n`, '\n')
  }

  renderSection('Unknown', notes.unknown)

  return rendered.join('')
}

/***
****  Module
***/

module.exports = {
  get: getNotes,
  render: renderNotes
}
