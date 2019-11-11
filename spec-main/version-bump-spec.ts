import { expect } from 'chai'
import { nextVersion } from '../script/release/version-bumper'
import * as utils from '../script/release/version-utils'
import { ifdescribe } from './spec-helpers'

describe('version-bumper', () => {
  describe('makeVersion', () => {
    it('makes a version with a period delimeter', () => {
      const components = {
        major: 2,
        minor: 0,
        patch: 0
      }

      const version = utils.makeVersion(components, '.')
      expect(version).to.equal('2.0.0')
    })

    it('makes a version with a period delimeter and a partial pre', () => {
      const components = {
        major: 2,
        minor: 0,
        patch: 0,
        pre: [ 'nightly', 12345678 ]
      }

      const version = utils.makeVersion(components, '.', utils.preType.PARTIAL)
      expect(version).to.equal('2.0.0.12345678')
    })

    it('makes a version with a period delimeter and a full pre', () => {
      const components = {
        major: 2,
        minor: 0,
        patch: 0,
        pre: [ 'nightly', 12345678 ]
      }

      const version = utils.makeVersion(components, '.', utils.preType.FULL)
      expect(version).to.equal('2.0.0-nightly.12345678')
    })
  })

  // On macOS Circle CI we don't have a real git environment due to running
  // gclient sync on a linux machine. These tests therefore don't run as expected.
  ifdescribe(!(process.platform === 'linux' && process.arch === 'arm') && process.platform !== 'darwin')('nextVersion', () => {
    const nightlyPattern = /[0-9.]*(-nightly.(\d{4})(\d{2})(\d{2}))$/g
    const betaPattern = /[0-9.]*(-beta[0-9.]*)/g

    it('bumps to nightly from stable', async () => {
      const version = 'v2.0.0'
      const next = await nextVersion('nightly', version)
      const matches = next.match(nightlyPattern)
      expect(matches).to.have.lengthOf(1)
    })

    it('bumps to nightly from beta', async () => {
      const version = 'v2.0.0-beta.1'
      const next = await nextVersion('nightly', version)
      const matches = next.match(nightlyPattern)
      expect(matches).to.have.lengthOf(1)
    })

    it('bumps to nightly from nightly', async () => {
      const version = 'v2.0.0-nightly.19950901'
      const next = await nextVersion('nightly', version)
      const matches = next.match(nightlyPattern)
      expect(matches).to.have.lengthOf(1)
    })

    it('bumps to a nightly version above our switch from N-0-x to N-x-y branch names', async () => {
      const version = 'v2.0.0-nightly.19950901'
      const next = await nextVersion('nightly', version)
      // If it starts with v8 then we didn't bump above the 8-x-y branch
      expect(next.startsWith('v8')).to.equal(false)
    })

    it('throws error when bumping to beta from stable', () => {
      const version = 'v2.0.0'
      return expect(
        nextVersion('beta', version)
      ).to.be.rejectedWith('Cannot bump to beta from stable.')
    })

    it('bumps to beta from nightly', async () => {
      const version = 'v2.0.0-nightly.19950901'
      const next = await nextVersion('beta', version)
      const matches = next.match(betaPattern)
      expect(matches).to.have.lengthOf(1)
    })

    it('bumps to beta from beta', async () => {
      const version = 'v2.0.0-beta.8'
      const next = await nextVersion('beta', version)
      expect(next).to.equal('2.0.0-beta.9')
    })

    it('bumps to stable from beta', async () => {
      const version = 'v2.0.0-beta.1'
      const next = await nextVersion('stable', version)
      expect(next).to.equal('2.0.0')
    })

    it('bumps to stable from stable', async () => {
      const version = 'v2.0.0'
      const next = await nextVersion('stable', version)
      expect(next).to.equal('2.0.1')
    })

    it('bumps to minor from stable', async () => {
      const version = 'v2.0.0'
      const next = await nextVersion('minor', version)
      expect(next).to.equal('2.1.0')
    })

    it('bumps to stable from nightly', async () => {
      const version = 'v2.0.0-nightly.19950901'
      const next = await nextVersion('stable', version)
      expect(next).to.equal('2.0.0')
    })

    it('throws on an invalid version', () => {
      const version = 'vI.AM.INVALID'
      return expect(
        nextVersion('beta', version)
      ).to.be.rejectedWith(`Invalid current version: ${version}`)
    })

    it('throws on an invalid bump type', () => {
      const version = 'v2.0.0'
      return expect(
        nextVersion('WRONG', version)
      ).to.be.rejectedWith('Invalid bump type.')
    })
  })
})
