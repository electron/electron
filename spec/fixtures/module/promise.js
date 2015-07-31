exports.twicePromise = function (promise) {
  return promise.then(function (value) {
    return value * 2;
  });
}
