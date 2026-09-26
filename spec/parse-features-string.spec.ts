import { describe, expect, it } from 'vitest';

import { createRequire } from 'node:module';

// See deprecate.spec.ts for why this is require()d rather than imported.
const { parseCommaSeparatedKeyValue, parseFeatures }: typeof import('../lib/browser/parse-features-string.ts') =
  createRequire(import.meta.url)('../lib/browser/parse-features-string.ts');

describe('feature-string parsing', () => {
  it('is indifferent to whitespace around keys and values', () => {
    const checkParse = (string: string, parsed: Record<string, string | boolean>) => {
      const features = parseCommaSeparatedKeyValue(string);
      expect(features).to.deep.equal(parsed);
    };
    checkParse('a=yes,c=d', { a: true, c: 'd' });
    checkParse('a=yes ,c=d', { a: true, c: 'd' });
    checkParse('a=yes, c=d', { a: true, c: 'd' });
    checkParse('a=yes , c=d', { a: true, c: 'd' });
    checkParse(' a=yes , c=d', { a: true, c: 'd' });
    checkParse(' a= yes , c=d', { a: true, c: 'd' });
    checkParse(' a = yes , c=d', { a: true, c: 'd' });
    checkParse(' a = yes , c =d', { a: true, c: 'd' });
    checkParse(' a = yes , c = d', { a: true, c: 'd' });
    checkParse(' a = yes , c = d ', { a: true, c: 'd' });
  });

  describe('parseFeatures allowlist', () => {
    it('passes through allowlisted presentational options', () => {
      const { options } = parseFeatures(
        'width=400,height=300,show=no,frame=no,title=hi,backgroundColor=#fff,left=10,top=20'
      );
      expect(options).to.deep.equal({
        width: 400,
        height: 300,
        show: false,
        frame: false,
        title: 'hi',
        backgroundColor: '#fff',
        left: 10,
        top: 20,
        x: 10,
        y: 20
      });
    });

    it('drops non-allowlisted options that would be unsafe to pass to BrowserWindow', () => {
      const { options } = parseFeatures('icon=/etc/passwd,width=400');
      expect(options).to.deep.equal({ width: 400 });
      expect(options).to.not.have.property('icon');
    });

    it('drops non-allowlisted options even when paired with UNC paths', () => {
      const { options } = parseFeatures('icon=\\\\attacker.example\\share\\x.png,show=no');
      expect(options).to.deep.equal({ show: false });
      expect(options).to.not.have.property('icon');
    });

    it('drops unknown options', () => {
      const { options } = parseFeatures('something-unknown=foo,width=400');
      expect(options).to.deep.equal({ width: 400 });
      expect(options).to.not.have.property('something-unknown');
    });

    it('still extracts allowlisted webPreferences', () => {
      const { options, webPreferences } = parseFeatures('icon=/etc/passwd,nodeIntegration=no,width=400');
      expect(options).to.deep.equal({ width: 400 });
      expect(webPreferences).to.deep.equal({ nodeIntegration: false });
    });
  });
});
