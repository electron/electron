import { afterEach, beforeEach, describe, expect, it } from 'vitest';

import { createRequire } from 'node:module';

// lib/ sits in a package scope with no "type", so import-ing its ESM-syntax
// sources makes Node reparse them with a MODULE_TYPELESS_PACKAGE_JSON warning;
// require() detects the module syntax quietly.
const deprecate: typeof import('../lib/common/deprecate.ts') = createRequire(import.meta.url)(
  '../lib/common/deprecate.ts'
);

describe('deprecate', () => {
  let throwing: boolean;

  beforeEach(() => {
    throwing = process.throwDeprecation;
    deprecate.setHandler(null);
    process.throwDeprecation = true;
  });

  afterEach(() => {
    process.throwDeprecation = throwing;
  });

  it('allows a deprecation handler function to be specified', () => {
    const messages: string[] = [];

    deprecate.setHandler((message) => {
      messages.push(message);
    });

    deprecate.log('this is deprecated');
    expect(messages).toEqual(['this is deprecated']);
  });

  it('returns a deprecation handler after one is set', () => {
    const messages = [];

    deprecate.setHandler((message) => {
      messages.push(message);
    });

    deprecate.log('this is deprecated');
    expect(deprecate.getHandler()).toBeTypeOf('function');
  });

  it('renames a property', () => {
    let msg;
    deprecate.setHandler((m) => {
      msg = m;
    });

    const oldProp = 'dingyOldName';
    const newProp = 'shinyNewName';

    let value = 0;
    const o: Record<string, number> = { [newProp]: value };
    expect(o).not.toHaveProperty([oldProp]);
    expect(o).toHaveProperty([newProp], expect.any(Number));

    deprecate.renameProperty(o, oldProp, newProp);
    o[oldProp] = ++value;

    expect(msg).toBeTypeOf('string');
    expect(msg).toContain(oldProp);
    expect(msg).toContain(newProp);

    expect(o).toHaveProperty([newProp], value);
    expect(o).toHaveProperty([oldProp], value);
  });

  it("doesn't deprecate a property not on an object", () => {
    const o: any = {};

    expect(() => {
      deprecate.removeProperty(o, 'iDoNotExist');
    }).toThrow(/iDoNotExist/);
  });

  it('deprecates a property of an object', () => {
    let msg;
    deprecate.setHandler((m) => {
      msg = m;
    });

    const prop = 'itMustGo';
    const o = { [prop]: 0 };

    deprecate.removeProperty(o, prop);

    const temp = o[prop];

    expect(temp).toBe(0);
    expect(msg).toBeTypeOf('string');
    expect(msg).toContain(prop);
  });

  it('deprecates a property of an but retains the existing accessors and setters', () => {
    let msg;
    deprecate.setHandler((m) => {
      msg = m;
    });

    const prop = 'itMustGo';
    let i = 1;
    const o = {
      get itMustGo() {
        return i;
      },
      set itMustGo(thing) {
        i = thing + 1;
      }
    };

    deprecate.removeProperty(o, prop);

    expect(o[prop]).toBe(1);
    expect(msg).toBeTypeOf('string');
    expect(msg).toContain(prop);
    o[prop] = 2;
    expect(o[prop]).toBe(3);
  });

  it('warns exactly once when a function is deprecated with no replacement', () => {
    let msg;
    deprecate.setHandler((m) => {
      msg = m;
    });

    function oldFn() {
      return 'hello';
    }
    const deprecatedFn = deprecate.removeFunction(oldFn, 'oldFn');
    deprecatedFn();

    expect(msg).toBeTypeOf('string');
    expect(msg).toContain('oldFn');
  });

  it('preserves the return value and `this` of a deprecated function with no replacement', () => {
    deprecate.setHandler(() => {});

    function oldFn(this: any, a: number, b: number) {
      return { self: this, sum: a + b };
    }
    const deprecatedFn = deprecate.removeFunction(oldFn, 'oldFn');
    const context = { name: 'ctx' };
    const result = deprecatedFn.call(context, 2, 3);

    expect(result).toEqual({ self: context, sum: 5 });
  });

  it('warns exactly once when a function is deprecated with a replacement', () => {
    let msg;
    deprecate.setHandler((m) => {
      msg = m;
    });

    function oldFn() {
      return 'hello';
    }
    const deprecatedFn = deprecate.renameFunction(oldFn, 'newFn');
    deprecatedFn();

    expect(msg).toBeTypeOf('string');
    expect(msg).toContain('oldFn');
    expect(msg).toContain('newFn');
  });

  it('warns only once per item', () => {
    const messages: string[] = [];
    deprecate.setHandler((message) => messages.push(message));

    const key = 'foo';
    const val = 'bar';
    const o = { [key]: val };
    deprecate.removeProperty(o, key);

    for (let i = 0; i < 3; ++i) {
      expect(o[key]).toBe(val);
      expect(messages).toHaveLength(1);
    }
  });

  it('warns if deprecated property is already set', () => {
    let msg;
    deprecate.setHandler((m) => {
      msg = m;
    });

    const oldProp = 'dingyOldName';
    const newProp = 'shinyNewName';

    const o: Record<string, number> = { [oldProp]: 0 };
    deprecate.renameProperty(o, oldProp, newProp);

    expect(msg).toBeTypeOf('string');
    expect(msg).toContain(oldProp);
    expect(msg).toContain(newProp);
  });

  it('throws an exception if no deprecation handler is specified', () => {
    expect(() => {
      deprecate.log('this is deprecated');
    }).toThrow(/this is deprecated/);
  });

  describe('moveAPI', () => {
    beforeEach(() => {
      deprecate.setHandler(null);
    });

    it('should call the original method', () => {
      const warnings = [];
      deprecate.setHandler((warning) => warnings.push(warning));

      let called = false;
      const fn = () => {
        called = true;
      };
      const deprecated = deprecate.moveAPI(fn, 'old', 'new');
      deprecated();
      expect(called).toBe(true);
    });

    it('should log the deprecation warning once', () => {
      const warnings: string[] = [];
      deprecate.setHandler((warning) => warnings.push(warning));

      const deprecated = deprecate.moveAPI(() => null, 'old', 'new');
      deprecated();
      expect(warnings).toHaveLength(1);
      deprecated();
      expect(warnings).toHaveLength(1);
      expect(warnings[0]).toBe("'old' is deprecated and will be removed. Please use 'new' instead.");
    });
  });
});
