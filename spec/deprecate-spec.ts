import { expect } from 'chai';

import { expectDeprecationMessages } from './lib/warning-helpers';
import * as deprecate from '../lib/common/deprecate';

describe('deprecate', () => {
  it('renames a property', async () => {
    const oldProp = 'dingyOldName';
    const newProp = 'shinyNewName';

    let value = 0;
    const o: Record<string, number> = { [newProp]: value };
    expect(o).to.not.have.property(oldProp);
    expect(o).to.have.property(newProp).that.is.a('number');

    deprecate.renameProperty(o, oldProp, newProp);

    await expectDeprecationMessages(
      () => {
        o[oldProp] = ++value;
      },
      `'${oldProp}' is deprecated and will be removed. Please use '${newProp}' instead.`
    );

    expect(o).to.have.property(newProp).that.is.equal(value);
    expect(o).to.have.property(oldProp).that.is.equal(value);
  });

  describe('removeProperty', () => {
    it('doesn\'t deprecate a property not on an object', async () => {
      const prop = 'iDoNotExist';
      const o: any = {};

      await expectDeprecationMessages(
        () => {
          deprecate.removeProperty(o, prop);
        },
        `Unable to remove property '${prop}' from an object that lacks it.`
      );
    });

    it('doesn\'t deprecate a property without getter / setter', async () => {
      const prop = 'itMustGo';
      const o = { [prop]: 0 };

      await expectDeprecationMessages(
        () => {
          deprecate.removeProperty(o, prop);
        },
        `Unable to remove property '${prop}' from an object does not have a getter / setter`
      );
    });

    it('deprecates a property of an object', async () => {
      const prop = 'itMustGo';
      const o = {
        get [prop] () { return 0; },
        set [prop] (_thing) {}
      };

      deprecate.removeProperty(o, prop);

      const temp = await expectDeprecationMessages(
        () => {
          return o[prop];
        },
        `'${prop}' is deprecated and will be removed.`
      );

      expect(temp).to.equal(0);
    });

    it('deprecates a property of an object but retains the existing accessors and setters', async () => {
      const prop = 'itMustGo';
      let i = 1;
      const o = {
        get [prop] () {
          return i;
        },
        set [prop] (thing) {
          i = thing + 1;
        }
      };

      deprecate.removeProperty(o, prop);

      await expectDeprecationMessages(
        () => {
          expect(o[prop]).to.equal(1);
        },
        `'${prop}' is deprecated and will be removed.`
      );

      o[prop] = 2;
      expect(o[prop]).to.equal(3);
    });

    it('deprecates a property of an object but only warns on setting specific values with onlyForValues', async () => {
      const prop = 'itMustGo';
      let i = 1;
      const o = {
        get [prop] () {
          return i;
        },
        set [prop] (thing) {
          i = thing;
        }
      };

      deprecate.removeProperty(o, prop, [42]);

      await expectDeprecationMessages(
        () => {
          o[prop] = 2;
        }
        // Expect no deprecation message
      );

      await expectDeprecationMessages(
        () => {
          o[prop] = 42;
        },
        `'${prop}' is deprecated and will be removed.`
      );
    });

    it('warns only once per item', async () => {
      const key = 'foo';
      const val = 'bar';
      const o = {
        get [key] () {
          return val;
        },
        set [key] (_thing) {}
      };
      deprecate.removeProperty(o, key);

      await expectDeprecationMessages(
        () => {
          for (let i = 0; i < 3; ++i) {
            expect(o[key]).to.equal(val);
          }
        },
        `'${key}' is deprecated and will be removed.`
      );
    });
  });

  it('warns exactly once when a function is deprecated with no replacement', async () => {
    function oldFn () { return 'hello'; }
    const deprecatedFn = deprecate.removeFunction(oldFn, 'oldFn');

    await expectDeprecationMessages(
      () => {
        deprecatedFn();
        deprecatedFn();
        deprecatedFn();
      },
      "'oldFn function' is deprecated and will be removed."
    );
  });

  it('warns exactly once when a function is deprecated with a replacement', async () => {
    function oldFn () { return 'hello'; }
    const deprecatedFn = deprecate.renameFunction(oldFn, 'newFn');

    await expectDeprecationMessages(
      () => {
        deprecatedFn();
        deprecatedFn();
        deprecatedFn();
      },
      "'oldFn function' is deprecated and will be removed. Please use 'newFn function' instead."
    );
  });

  it('warns if deprecated property is already set', async () => {
    const oldProp = 'dingyOldName';
    const newProp = 'shinyNewName';

    const o: Record<string, number> = { [oldProp]: 0 };

    await expectDeprecationMessages(
      () => {
        deprecate.renameProperty(o, oldProp, newProp);
      },
      `'${oldProp}' is deprecated and will be removed. Please use '${newProp}' instead.`
    );
  });

  it('does not warn if process.noDeprecation is true', async () => {
    process.noDeprecation = true;

    try {
      function oldFn () { return 'hello'; }
      const deprecatedFn = deprecate.removeFunction(oldFn, 'oldFn');

      await expectDeprecationMessages(
        () => {
          deprecatedFn();
        }
        // Expect no deprecation message
      );
    } finally {
      process.noDeprecation = false;
    }
  });

  describe('moveAPI', () => {
    it('should call the original method', () => {
      let called = false;
      const fn = () => {
        called = true;
      };
      const deprecated = deprecate.moveAPI(fn, 'old', 'new');
      deprecated();
      expect(called).to.equal(true);
    });

    it('should log the deprecation warning once', async () => {
      const deprecated = deprecate.moveAPI(() => null, 'old', 'new');

      await expectDeprecationMessages(
        () => {
          deprecated();
          deprecated();
        },
        "'old' is deprecated and will be removed. Please use 'new' instead."
      );
    });
  });
});
