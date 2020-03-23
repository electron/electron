import { expect } from 'chai';

describe('feature-string parsing', () => {
  it('is indifferent to whitespace around keys and values', () => {
    const { parseFeaturesString } = require('../lib/common/parse-features-string');
    const checkParse = (string: string, parsed: Record<string, string | boolean>) => {
      const features: Record<string, string | boolean> = {};
      parseFeaturesString(string, (k: string, v: any) => { features[k] = v; });
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
