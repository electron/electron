import { expect } from 'chai';

describe('feature-string parsing', () => {
  it('is indifferent to whitespace around keys and values', () => {
    const { parseCommaSeparatedKeyValue } = require('../lib/browser/parse-features-string');
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
});
