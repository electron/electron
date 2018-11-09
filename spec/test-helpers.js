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

module.exports = {
  platformDescribe,
  platformIt
}
