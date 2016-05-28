const foo = Object.create(null)
foo.bar = 'baz'
foo.baz = false
module.exports = {foo: foo, bar: 1234}
