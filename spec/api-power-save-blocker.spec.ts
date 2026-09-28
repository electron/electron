import { powerSaveBlocker } from 'electron/main';

import { expect } from 'chai';

describe('powerSaveBlocker module', () => {
  it('can be started and stopped', () => {
    expect(powerSaveBlocker.isStarted(-1)).to.be.false('is started');
    const id = powerSaveBlocker.start('prevent-app-suspension');
    expect(id).to.to.be.a('number');
    expect(powerSaveBlocker.isStarted(id)).to.be.true('is started');
    powerSaveBlocker.stop(id);
    expect(powerSaveBlocker.isStarted(id)).to.be.false('is started');
  });
});
