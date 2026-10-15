import { describe, expect, it } from 'vitest';

import * as path from 'node:path';

import { spawnAndWait } from './lib/spec-helpers.ts';

describe('native VideoFrame handles', () => {
  const fixture = path.join(import.meta.dirname, 'fixtures/api/native-video-frame/main.cjs');

  it('rejects invalid, closed and CPU frames from a sandboxed preload', async () => {
    const result = await spawnAndWait(process.execPath, [fixture], {
      env: { ...process.env, ELECTRON_NATIVE_FRAME_HARDWARE_TESTS: '0' },
      timeout: 30000
    });
    expect(result.code, result.stderr).toBe(0);
    expect(result.stdout).toContain('"paintEvents":0');
  }, 40000);

  it.skipIf(process.platform !== 'linux' || process.env.ELECTRON_NATIVE_FRAME_HARDWARE_TESTS !== '1')(
    'preserves native pixels, ownership and release without paint on Linux',
    async () => {
      const result = await spawnAndWait(process.execPath, [fixture], {
        env: { ...process.env, ELECTRON_NATIVE_FRAME_HARDWARE_TESTS: '1' },
        timeout: 45000
      });
      expect(result.code, result.stderr).toBe(0);
      expect(result.stdout).toContain('"paintEvents":0');
    },
    55000
  );
});
