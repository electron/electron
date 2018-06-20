var test = 'test'

const types = {
  require: typeof require,
  electron: typeof electron,
  window: typeof window,
  localVar: typeof window.test
}

console.log(JSON.stringify(types))
