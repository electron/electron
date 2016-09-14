exports.setup = function () {
  const foo = {}

  foo.bar = function () {
    return delete foo.bar.baz && delete foo.bar
  }

  foo.bar.baz = function () {
    return 3
  }

  return foo
}
