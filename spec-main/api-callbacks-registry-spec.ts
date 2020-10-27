import { expect } from 'chai';
import { CallbacksRegistry } from '../lib/renderer/remote/callbacks-registry';
import { ifdescribe } from './spec-helpers';

const features = process._linkedBinding('electron_common_features');

ifdescribe(features.isRemoteModuleEnabled())('CallbacksRegistry module', () => {
  let registry: CallbacksRegistry;

  beforeEach(() => {
    registry = new CallbacksRegistry();
  });

  it('adds a callback to the registry', () => {
    const cb = () => [1, 2, 3, 4, 5];
    const key = registry.add(cb);

    expect(key).to.exist('key');
  });

  it('returns a specified callback if it is in the registry', () => {
    const cb = () => [1, 2, 3, 4, 5];
    const key = registry.add(cb);
    expect(key).to.exist('key');
    const callback = registry.get(key!);

    expect(callback.toString()).equal(cb.toString());
  });

  it('returns an empty function if the cb doesnt exist', () => {
    const callback = registry.get(1);

    expect(callback).to.be.a('function');
  });

  it('removes a callback to the registry', () => {
    const cb = () => [1, 2, 3, 4, 5];
    const key = registry.add(cb);
    expect(key).to.exist('key');

    registry.remove(key!);
    const afterCB = registry.get(key!);

    expect(afterCB).to.be.a('function');
    expect(afterCB.toString()).to.not.equal(cb.toString());
  });
});
