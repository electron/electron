const foo = Object.create(null)
foo.bar = 'baz'
foo.baz = false
module.exports = {
  foo: foo,
  bar: 1234,
  getConstructorName: function (value) {
    return value.constructor.name
  }
}
