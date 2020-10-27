class Foo {
  set bar (value) {
    throw new Error('setting error');
  }

  get bar () {
    throw new Error('getting error');
  }
}

module.exports = new Foo();
