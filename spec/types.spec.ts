import { describe, expect, it } from 'vitest';

import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);

describe('bundled @types/node', () => {
  it('should match the major version of bundled node', () => {
    expect(require('../npm/package.json').dependencies).toHaveProperty('@types/node');
    const range = require('../npm/package.json').dependencies['@types/node'];
    expect(range, 'should allow any type dep in a major range').toMatch(/^\^.+/);
    expect(range.slice(1).split('.')[0]).toBe(process.versions.node.split('.')[0]);
  });
});
