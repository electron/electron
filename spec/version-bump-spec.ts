import { expect } from 'chai';
import { GitProcess, IGitExecutionOptions, IGitResult } from 'dugite';
import { nextVersion } from '../script/release/version-bumper';
import * as utils from '../script/release/version-utils';
import * as sinon from 'sinon';
import { ifdescribe } from './lib/spec-helpers';

class GitFake {
  branches: {
    [key: string]: string[],
  };

  constructor () {
    this.branches = {};
  }

  setBranch (channel: string): void {
    this.branches[channel] = [];
  }

  setVersion (channel: string, latestTag: string): void {
    const tags = [latestTag];
    if (channel === 'alpha') {
      const versionStrs = latestTag.split(`${channel}.`);
      const latest = parseInt(versionStrs[1]);

      for (let i = latest; i >= 1; i--) {
        tags.push(`${versionStrs[0]}${channel}.${latest - i}`);
      }
    }

    this.branches[channel] = tags;
  }

  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  exec (args: string[], path: string, options?: IGitExecutionOptions | undefined): Promise<IGitResult> {
    let stdout = '';
    const stderr = '';
    const exitCode = 0;

    // handle for promoting from current master HEAD
    let branch = 'stable';
    const v = (args[2] === 'HEAD') ? 'stable' : args[3];
    if (v.includes('nightly')) branch = 'nightly';
    if (v.includes('alpha')) branch = 'alpha';
    if (v.includes('beta')) branch = 'beta';

    if (!this.branches[branch]) this.setBranch(branch);

    stdout = this.branches[branch].join('\n');
    return Promise.resolve({ exitCode, stdout, stderr });
  }
}

describe('version-bumper', () => {
  describe('makeVersion', () => {
    it('makes a version with a period delimiter', () => {
      const components = {
        major: 2,
        minor: 0,
        patch: 0
      };

      const version = utils.makeVersion(components, '.');
      expect(version).to.equal('2.0.0');
    });

    it('makes a version with a period delimiter and a partial pre', () => {
      const components = {
        major: 2,
        minor: 0,
        patch: 0,
        pre: ['nightly', 12345678]
      };

      const version = utils.makeVersion(components, '.', utils.preType.PARTIAL);
      expect(version).to.equal('2.0.0.12345678');
    });

    it('makes a version with a period delimiter and a full pre', () => {
      const components = {
        major: 2,
        minor: 0,
        patch: 0,
        pre: ['nightly', 12345678]
      };

      const version = utils.makeVersion(components, '.', utils.preType.FULL);
      expect(version).to.equal('2.0.0-nightly.12345678');
    });
  });

  // On macOS Circle CI we don't have a real git environment due to running
  // gclient sync on a linux machine. These tests therefore don't run as expected.
  ifdescribe(!(process.platform === 'linux' && process.arch.indexOf('arm') === 0) && process.platform !== 'darwin')('nextVersion', () => {
    describe('bump versions', () => {
      const nightlyPattern = /[0-9.]*(-nightly.(\d{4})(\d{2})(\d{2}))$/g;
      const betaPattern = /[0-9.]*(-beta[0-9.]*)/g;

      it('bumps to nightly from stable', async () => {
        const version = 'v2.0.0';
        const next = await nextVersion('nightly', version);
        const matches = next.match(nightlyPattern);
        expect(matches).to.have.lengthOf(1);
      });

      it('bumps to nightly from beta', async () => {
        const version = 'v2.0.0-beta.1';
        const next = await nextVersion('nightly', version);
        const matches = next.match(nightlyPattern);
        expect(matches).to.have.lengthOf(1);
      });

      it('bumps to nightly from nightly', async () => {
        const version = 'v2.0.0-nightly.19950901';
        const next = await nextVersion('nightly', version);
        const matches = next.match(nightlyPattern);
        expect(matches).to.have.lengthOf(1);
      });

      it('bumps to a nightly version above our switch from N-0-x to N-x-y branch names', async () => {
        const version = 'v2.0.0-nightly.19950901';
        const next = await nextVersion('nightly', version);
        // If it starts with v8 then we didn't bump above the 8-x-y branch
        expect(next.startsWith('v8')).to.equal(false);
      });

      it('throws error when bumping to beta from stable', () => {
        const version = 'v2.0.0';
        return expect(
          nextVersion('beta', version)
        ).to.be.rejectedWith('Cannot bump to beta from stable.');
      });

      it('bumps to beta from nightly', async () => {
        const version = 'v2.0.0-nightly.19950901';
        const next = await nextVersion('beta', version);
        const matches = next.match(betaPattern);
        expect(matches).to.have.lengthOf(1);
      });

      it('bumps to beta from beta', async () => {
        const version = 'v2.0.0-beta.8';
        const next = await nextVersion('beta', version);
        expect(next).to.equal('2.0.0-beta.9');
      });

      it('bumps to beta from beta if the previous beta is at least beta.10', async () => {
        const version = 'v6.0.0-beta.15';
        const next = await nextVersion('beta', version);
        expect(next).to.equal('6.0.0-beta.16');
      });

      it('bumps to stable from beta', async () => {
        const version = 'v2.0.0-beta.1';
        const next = await nextVersion('stable', version);
        expect(next).to.equal('2.0.0');
      });

      it('bumps to stable from stable', async () => {
        const version = 'v2.0.0';
        const next = await nextVersion('stable', version);
        expect(next).to.equal('2.0.1');
      });

      it('bumps to minor from stable', async () => {
        const version = 'v2.0.0';
        const next = await nextVersion('minor', version);
        expect(next).to.equal('2.1.0');
      });

      it('bumps to stable from nightly', async () => {
        const version = 'v2.0.0-nightly.19950901';
        const next = await nextVersion('stable', version);
        expect(next).to.equal('2.0.0');
      });

      it('throws on an invalid version', () => {
        const version = 'vI.AM.INVALID';
        return expect(
          nextVersion('beta', version)
        ).to.be.rejectedWith(`Invalid current version: ${version}`);
      });

      it('throws on an invalid bump type', () => {
        const version = 'v2.0.0';
        return expect(
          nextVersion('WRONG', version)
        ).to.be.rejectedWith('Invalid bump type.');
      });
    });
  });

  // If we don't plan on continuing to support an alpha channel past Electron 15,
  // these tests will be removed. Otherwise, integrate into the bump versions tests
  describe('bump versions - alpha channel', () => {
    const alphaPattern = /[0-9.]*(-alpha[0-9.]*)/g;
    const betaPattern = /[0-9.]*(-beta[0-9.]*)/g;

    const sandbox = sinon.createSandbox();
    const gitFake = new GitFake();

    beforeEach(() => {
      const wrapper = (args: string[], path: string, options?: IGitExecutionOptions | undefined) => gitFake.exec(args, path, options);
      sandbox.replace(GitProcess, 'exec', wrapper);
    });

    afterEach(() => {
      gitFake.branches = {};
      sandbox.restore();
    });

    it('bumps to alpha from nightly', async () => {
      const version = 'v2.0.0-nightly.19950901';
      gitFake.setVersion('nightly', version);
      const next = await nextVersion('alpha', version);
      const matches = next.match(alphaPattern);
      expect(matches).to.have.lengthOf(1);
    });

    it('throws error when bumping to alpha from stable', () => {
      const version = 'v2.0.0';
      return expect(
        nextVersion('alpha', version)
      ).to.be.rejectedWith('Cannot bump to alpha from stable.');
    });

    it('bumps to alpha from alpha', async () => {
      const version = 'v2.0.0-alpha.8';
      gitFake.setVersion('alpha', version);
      const next = await nextVersion('alpha', version);
      expect(next).to.equal('2.0.0-alpha.9');
    });

    it('bumps to alpha from alpha if the previous alpha is at least alpha.10', async () => {
      const version = 'v6.0.0-alpha.15';
      gitFake.setVersion('alpha', version);
      const next = await nextVersion('alpha', version);
      expect(next).to.equal('6.0.0-alpha.16');
    });

    it('bumps to beta from alpha', async () => {
      const version = 'v2.0.0-alpha.8';
      gitFake.setVersion('alpha', version);
      const next = await nextVersion('beta', version);
      const matches = next.match(betaPattern);
      expect(matches).to.have.lengthOf(1);
      expect(next).to.equal('2.0.0-beta.1');
    });
  });
});
