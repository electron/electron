function foo () {
  return 'hello'
}
foo.bar = 'baz'
foo.nested = {
  prop: 'yes'
}
foo.method1 = function () {
  return 'world'
}
foo.method1.prop1 = function () {
  return 123
}

module.exports = {
  foo: foo
}
