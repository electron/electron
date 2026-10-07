import { expect } from 'chai';

import * as fs from 'node:fs';
import * as path from 'node:path';

import { ifdescribe, ifit, isTestingBindingAvailable, startRemoteControlApp } from './lib/spec-helpers.ts';

interface CacheResult {
  matchingAccepted: boolean;
  matchingValue: number;
  foreignAccepted: boolean;
  foreignValue: number;
  rebuiltAccepted: boolean;
  rebuiltValue: number;
}

const fixture = path.resolve(import.meta.dirname, 'fixtures/api/v8-code-cache.cjs');
const buildDir =
  process.platform === 'darwin'
    ? path.resolve(path.dirname(process.execPath), '../../..')
    : path.dirname(process.execPath);
const builder = path.join(buildDir, 'gen', 'electron', 'v8-code-cache', 'wasm-module-builder.js');

describe('V8 patch code cache compatibility', () => {
  it('exposes a nonempty V8 version with a 16-hex Electron patch fingerprint', () => {
    expect(process.versions.v8).to.be.a('string').and.not.be.empty();
    expect(process.versions.v8).to.match(/^\d+\.\d+\.\d+(?:\.\d+)?-electron\.[0-9a-f]{16}(?: \(candidate\))?$/);
  });

  ifdescribe(isTestingBindingAvailable())('serialized caches', () => {
    it('rejects JavaScript caches from a different patch fingerprint and falls back to source', async () => {
      const { remotely } = await startRemoteControlApp();
      const result: CacheResult = await remotely((file: string) => require(file).javascript(), fixture);
      expect(result).to.deep.equal({
        matchingAccepted: true,
        matchingValue: 42,
        foreignAccepted: false,
        foreignValue: 42,
        rebuiltAccepted: true,
        rebuiltValue: 42
      });
    });

    ifit(fs.existsSync(builder))(
      'rejects serialized WebAssembly from a different patch fingerprint and falls back safely',
      async () => {
        const { remotely } = await startRemoteControlApp(['--js-flags=--no-liftoff --no-wasm-lazy-compilation']);
        const result: CacheResult = await remotely(
          (file: string, helper: string) => require(file).webassembly(helper),
          fixture,
          builder
        );
        expect(result).to.deep.equal({
          matchingAccepted: true,
          matchingValue: 1,
          foreignAccepted: false,
          foreignValue: 1,
          rebuiltAccepted: true,
          rebuiltValue: 1
        });
      }
    );
  });
});
