import { powerSaveBlocker } from 'electron/main';

import { describe, expect, it } from 'vitest';

describe('powerSaveBlocker module', () => {
  it('can be started and stopped', () => {
    expect(powerSaveBlocker.isStarted(-1), 'is started').to.be.false;
    const id = powerSaveBlocker.start('prevent-app-suspension');
    expect(id).to.to.be.a('number');
    expect(powerSaveBlocker.isStarted(id), 'is started').to.be.true;
    powerSaveBlocker.stop(id);
    expect(powerSaveBlocker.isStarted(id), 'is started').to.be.false;
  });
});
