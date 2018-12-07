const platformDependent = ([goodPlatformFn, skipFn, onlyFn]) => {
  const resultFn = (goodFn, badFn) => (name, platforms, testFn) => {
    let fn = goodPlatformFn
    if (!platforms.includes(process.platform)) {
      fn = skipFn
    }
    return fn(name, testFn);
  }
  const returnFn = resultFn(goodPlatformFn, skipFn);
  returnFn.skip = resultFn(skipFn, skipFn);
  returnFn.only = resultFn(onlyFn, skipFn);
  return returnFn;
}

const platformDescribe =  platformDependent([describe, describe.skip, describe.only])

const platformIt = platformDependent([it, it.skip, it.only])

// eslint-disable standard/no-unused-vars
const xplatformDescribe = platformDescribe.skip

// eslint-disable standard/no-unused-vars
const xplatformIt = platformIt.skip

module.exports = {
  platformDescribe,
  xplatformDescribe,
  platformIt,
  xplatformIt
}
