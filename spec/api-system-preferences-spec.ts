import { expect } from 'chai';
import { systemPreferences } from 'electron/main';
import { ifdescribe } from './lib/spec-helpers';

describe('systemPreferences module', () => {
  ifdescribe(process.platform === 'win32')('systemPreferences.getAccentColor', () => {
    it('should return a non-empty string', () => {
      const accentColor = systemPreferences.getAccentColor();
      expect(accentColor).to.be.a('string').that.is.not.empty('accent color');
    });
  });

  ifdescribe(process.platform === 'win32')('systemPreferences.getColor(id)', () => {
    it('throws an error when the id is invalid', () => {
      expect(() => {
        systemPreferences.getColor('not-a-color' as any);
      }).to.throw('Unknown color: not-a-color');
    });

    it('returns a hex RGBA color string', () => {
      expect(systemPreferences.getColor('window')).to.match(/^#[0-9A-F]{8}$/i);
    });
  });

  ifdescribe(process.platform === 'darwin')('systemPreferences.registerDefaults(defaults)', () => {
    it('registers defaults', () => {
      const defaultsMap = [
        { key: 'one', type: 'string', value: 'ONE' },
        { key: 'two', value: 2, type: 'integer' },
        { key: 'three', value: [1, 2, 3], type: 'array' }
      ] as const;

      const defaultsDict: Record<string, any> = {};
      for (const row of defaultsMap) { defaultsDict[row.key] = row.value; }

      systemPreferences.registerDefaults(defaultsDict);

      for (const userDefault of defaultsMap) {
        const { key, value: expectedValue, type } = userDefault;
        const actualValue = systemPreferences.getUserDefault(key, type);
        expect(actualValue).to.deep.equal(expectedValue);
      }
    });

    it('throws when bad defaults are passed', () => {
      const badDefaults = [
        1,
        null,
        { one: null }
      ];

      for (const badDefault of badDefaults) {
        expect(() => {
          systemPreferences.registerDefaults(badDefault as any);
        }).to.throw('Error processing argument at index 0, conversion failure from ');
      }
    });
  });

  ifdescribe(process.platform === 'darwin')('systemPreferences.getUserDefault(key, type)', () => {
    it('returns values for known user defaults', () => {
      const locale = systemPreferences.getUserDefault('AppleLocale', 'string');
      expect(locale).to.be.a('string').that.is.not.empty('locale');

      const languages = systemPreferences.getUserDefault('AppleLanguages', 'array');
      expect(languages).to.be.an('array').that.is.not.empty('languages');
    });

    it('returns values for unknown user defaults', () => {
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'boolean')).to.equal(false);
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'integer')).to.equal(0);
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'float')).to.equal(0);
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'double')).to.equal(0);
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'string')).to.equal('');
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'url')).to.equal('');
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'badtype' as any)).to.be.undefined('user default');
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'array')).to.deep.equal([]);
      expect(systemPreferences.getUserDefault('UserDefaultDoesNotExist', 'dictionary')).to.deep.equal({});
    });
  });

  ifdescribe(process.platform === 'darwin')('systemPreferences.setUserDefault(key, type, value)', () => {
    const KEY = 'SystemPreferencesTest';
    const TEST_CASES = [
      ['string', 'abc'],
      ['boolean', true],
      ['float', 2.5],
      ['double', 10.1],
      ['integer', 11],
      ['url', 'https://github.com/electron'],
      ['array', [1, 2, 3]],
      ['dictionary', { a: 1, b: 2 }]
    ] as const;

    it('sets values', () => {
      for (const [type, value] of TEST_CASES) {
        systemPreferences.setUserDefault(KEY, type, value as any);
        const retrievedValue = systemPreferences.getUserDefault(KEY, type);
        expect(retrievedValue).to.deep.equal(value);
      }
    });

    it('throws when type and value conflict', () => {
      for (const [type, value] of TEST_CASES) {
        expect(() => {
          systemPreferences.setUserDefault(KEY, type, typeof value === 'string' ? {} : 'foo' as any);
        }).to.throw(`Unable to convert value to: ${type}`);
      }
    });

    it('throws when type is not valid', () => {
      expect(() => {
        systemPreferences.setUserDefault(KEY, 'abc' as any, 'foo');
      }).to.throw('Invalid type: abc');
    });
  });

  ifdescribe(process.platform === 'darwin')('systemPreferences.subscribeNotification(event, callback)', () => {
    it('throws an error if invalid arguments are passed', () => {
      const badArgs = [123, {}, ['hi', 'bye'], new Date()];
      for (const bad of badArgs) {
        expect(() => {
          systemPreferences.subscribeNotification(bad as any, () => {});
        }).to.throw('Must pass null or a string');
      }
    });
  });

  ifdescribe(process.platform === 'darwin')('systemPreferences.subscribeLocalNotification(event, callback)', () => {
    it('throws an error if invalid arguments are passed', () => {
      const badArgs = [123, {}, ['hi', 'bye'], new Date()];
      for (const bad of badArgs) {
        expect(() => {
          systemPreferences.subscribeNotification(bad as any, () => {});
        }).to.throw('Must pass null or a string');
      }
    });
  });

  ifdescribe(process.platform === 'darwin')('systemPreferences.subscribeWorkspaceNotification(event, callback)', () => {
    it('throws an error if invalid arguments are passed', () => {
      const badArgs = [123, {}, ['hi', 'bye'], new Date()];
      for (const bad of badArgs) {
        expect(() => {
          systemPreferences.subscribeWorkspaceNotification(bad as any, () => {});
        }).to.throw('Must pass null or a string');
      }
    });
  });

  ifdescribe(process.platform === 'darwin')('systemPreferences.getSystemColor(color)', () => {
    it('throws on invalid system colors', () => {
      const color = 'bad-color';
      expect(() => {
        systemPreferences.getSystemColor(color as any);
      }).to.throw(`Unknown system color: ${color}`);
    });

    it('returns a valid system color', () => {
      const colors = ['blue', 'brown', 'gray', 'green', 'orange', 'pink', 'purple', 'red', 'yellow'] as const;

      for (const color of colors) {
        const sysColor = systemPreferences.getSystemColor(color);
        expect(sysColor).to.be.a('string');
      }
    });
  });

  ifdescribe(process.platform === 'darwin')('systemPreferences.getColor(color)', () => {
    it('throws on invalid colors', () => {
      const color = 'bad-color';
      expect(() => {
        systemPreferences.getColor(color as any);
      }).to.throw(`Unknown color: ${color}`);
    });

    it('returns a valid color', async () => {
      const colors = [
        'control-background',
        'control',
        'control-text',
        'disabled-control-text',
        'find-highlight',
        'grid',
        'header-text',
        'highlight',
        'keyboard-focus-indicator',
        'label',
        'link',
        'placeholder-text',
        'quaternary-label',
        'scrubber-textured-background',
        'secondary-label',
        'selected-content-background',
        'selected-control',
        'selected-control-text',
        'selected-menu-item-text',
        'selected-text-background',
        'selected-text',
        'separator',
        'shadow',
        'tertiary-label',
        'text-background',
        'text',
        'under-page-background',
        'unemphasized-selected-content-background',
        'unemphasized-selected-text-background',
        'unemphasized-selected-text',
        'window-background',
        'window-frame-text'
      ] as const;

      for (const color of colors) {
        const sysColor = systemPreferences.getColor(color);
        expect(sysColor).to.be.a('string');
      }
    });
  });

  ifdescribe(process.platform === 'darwin')('systemPreferences.effectiveAppearance', () => {
    const options = ['dark', 'light', 'unknown'];
    describe('with properties', () => {
      it('returns a valid appearance', () => {
        const appearance = systemPreferences.effectiveAppearance;
        expect(options).to.include(appearance);
      });
    });

    describe('with functions', () => {
      it('returns a valid appearance', () => {
        const appearance = systemPreferences.getEffectiveAppearance();
        expect(options).to.include(appearance);
      });
    });
  });

  ifdescribe(process.platform === 'darwin')('systemPreferences.setUserDefault(key, type, value)', () => {
    it('removes keys', () => {
      const KEY = 'SystemPreferencesTest';
      systemPreferences.setUserDefault(KEY, 'string', 'foo');
      systemPreferences.removeUserDefault(KEY);
      expect(systemPreferences.getUserDefault(KEY, 'string')).to.equal('');
    });

    it('does not throw for missing keys', () => {
      systemPreferences.removeUserDefault('some-missing-key');
    });
  });

  ifdescribe(process.platform === 'darwin')('systemPreferences.canPromptTouchID()', () => {
    it('returns a boolean', () => {
      expect(systemPreferences.canPromptTouchID()).to.be.a('boolean');
    });
  });

  ifdescribe(process.platform === 'darwin')('systemPreferences.isTrustedAccessibilityClient(prompt)', () => {
    it('returns a boolean', () => {
      const trusted = systemPreferences.isTrustedAccessibilityClient(false);
      expect(trusted).to.be.a('boolean');
    });
  });

  ifdescribe(['win32', 'darwin'].includes(process.platform))('systemPreferences.getMediaAccessStatus(mediaType)', () => {
    const statuses = ['not-determined', 'granted', 'denied', 'restricted', 'unknown'];

    it('returns an access status for a camera access request', () => {
      const cameraStatus = systemPreferences.getMediaAccessStatus('camera');
      expect(statuses).to.include(cameraStatus);
    });

    it('returns an access status for a microphone access request', () => {
      const microphoneStatus = systemPreferences.getMediaAccessStatus('microphone');
      expect(statuses).to.include(microphoneStatus);
    });

    it('returns an access status for a screen access request', () => {
      const screenStatus = systemPreferences.getMediaAccessStatus('screen');
      expect(statuses).to.include(screenStatus);
    });
  });

  describe('systemPreferences.getAnimationSettings()', () => {
    it('returns an object with all properties', () => {
      const settings = systemPreferences.getAnimationSettings();
      expect(settings).to.be.an('object');
      expect(settings).to.have.property('shouldRenderRichAnimation').that.is.a('boolean');
      expect(settings).to.have.property('scrollAnimationsEnabledBySystem').that.is.a('boolean');
      expect(settings).to.have.property('prefersReducedMotion').that.is.a('boolean');
    });
  });
});
