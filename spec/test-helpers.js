const platformDependent = (platforms, [goodPlatformFn, badPlatformFn], args) => {
  let fn = goodPlatformFn
  if (!platforms.includes(process.platform)) {
    fn = badPlatformFn
  }
  return fn(...args)
}

const platformDescribe = (name, platforms, fn) => {
  return platformDependent(platforms, [describe, describe.skip], [name, fn])
}

const platformIt = (name, platforms, fn) => {
  return platformDependent(platforms, [it, it.skip], [name, fn])
}

// eslint-disable standard/no-unused-vars
const xplatformDescribe = (name, platforms, fn) => {
  return xdescribe([name, fn])
}

// eslint-disable standard/no-unused-vars
const xplatformIt = (name, platforms, fn) => {
  return xit([name, fn])
}

module.exports = {
  platformDescribe,
  xplatformDescribe,
  platformIt,
  xplatformIt
}
