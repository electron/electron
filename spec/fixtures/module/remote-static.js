class Foo {
  static foo() {
    return 3
  }

  baz() {
    return 123
  }
}

Foo.bar = 'baz'

module.exports = {
  Foo: Foo
}
