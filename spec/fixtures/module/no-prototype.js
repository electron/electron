const foo = Object.create(null)
foo.bar = 'baz'
foo.baz = false
module.exports = {
  foo,
  bar: 1234,
  anonymous: new (class {})(),
  getConstructorName (value) {
    return value.constructor.name
  }
}
