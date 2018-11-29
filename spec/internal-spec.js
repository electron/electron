const chai = require('chai')
const { expect } = chai

describe('feature-string parsing', () => {
  it('is indifferent to whitespace around keys and values', () => {
    const parseFeaturesString = require('@electron/internal/common/parse-features-string')
    const checkParse = (string, parsed) => {
      const features = {}
      parseFeaturesString(string, (k, v) => { features[k] = v })
      expect(features).to.deep.equal(parsed)
    }
    checkParse('a=yes,c=d', { a: true, c: 'd' })
    checkParse('a=yes ,c=d', { a: true, c: 'd' })
    checkParse('a=yes, c=d', { a: true, c: 'd' })
    checkParse('a=yes , c=d', { a: true, c: 'd' })
    checkParse(' a=yes , c=d', { a: true, c: 'd' })
    checkParse(' a= yes , c=d', { a: true, c: 'd' })
    checkParse(' a = yes , c=d', { a: true, c: 'd' })
    checkParse(' a = yes , c =d', { a: true, c: 'd' })
    checkParse(' a = yes , c = d', { a: true, c: 'd' })
    checkParse(' a = yes , c = d ', { a: true, c: 'd' })
  })
})
