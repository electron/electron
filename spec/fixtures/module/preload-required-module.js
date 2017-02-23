try {
  console.log([typeof process, typeof setImmediate, typeof global, typeof Buffer, typeof global.Buffer].join(' '))
} catch (e) {
  console.log(e.message)
}
