import { expect } from 'chai';

describe('bundled @types/node', () => {
  it('should match the major version of bundled node', () => {
    expect(require('../npm/package.json').dependencies).to.have.property('@types/node');
    const range = require('../npm/package.json').dependencies['@types/node'];
    expect(range).to.match(/^\^.+/, 'should allow any type dep in a major range');
    expect(range.slice(1).split('.')[0]).to.equal(process.versions.node.split('.')[0]);
  });
});
