exports.reject = function (promise) {
  return promise.then(function () {
    throw Error('rejected')
  })
}
