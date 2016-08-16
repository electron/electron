function foo () {}
foo.bar = 'baz'
foo.nested = {
  prop: 'yes'
}

module.exports = {
  foo: foo
}
