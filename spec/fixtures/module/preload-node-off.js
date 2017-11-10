setImmediate(function () {
  try {
    console.log([typeof process, typeof setImmediate, typeof global, typeof Buffer].join(' '))
  } catch (e) {
    console.log(e.message)
  }
})
