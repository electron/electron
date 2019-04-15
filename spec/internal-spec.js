const chai = require('chai')
const { expect } = chai

describe('feature-string parsing', () => {
  const parseFeaturesString = require('@electron/internal/common/parse-features-string')
  const checkParse = (string, parsed) => {
    const features = {}
    parseFeaturesString(string, (k, v) => { features[k] = v })
    expect(features).to.deep.equal(parsed)
  }

  it('is indifferent to whitespace around keys and values', () => {
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
    checkParse('a=1, c=d, additionalArguments=[--name=name test,--title=title]', {
      additionalArguments: ['--name=name test', '--title=title'],
      a: true,
      c: 'd'
    })
    checkParse(' additionalArguments = [--title=title] ', {
      additionalArguments: ['--title=title']
    })
  })

  it('A name by itself is given a true boolean value', () => {
    checkParse('a', { a: true })
    checkParse('a, c', { a: true, c: true })
    checkParse('a,c=1', { a: true, c: true })
  })
})
