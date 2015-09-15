setImmediate(function() {
  try {
    console.log([typeof process, typeof setImmediate, typeof global].join(' '));
  } catch (e) {
    console.log(e.message);
  }
});
