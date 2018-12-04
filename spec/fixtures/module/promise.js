const v8Util = process.electronBinding('v8_util')

exports.twicePromise = function (promise) {
  return promise.then(function (value) {
    return value * 2
  })
}

exports.rejectPromise = function (promise) {
  return promise.then(function () {
    throw Error('rejected')
  })
}

exports.reject = function () {
  return Promise.reject(new Error('rejected'))
}

function markPromisified (func) {
  v8Util.setHiddenValue(func, 'promisified', true)
}

markPromisified(exports.twicePromise)
markPromisified(exports.rejectPromise)
markPromisified(exports.reject)
