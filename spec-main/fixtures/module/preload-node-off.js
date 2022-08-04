setImmediate(function () {
  try {
    const types = {
      process: typeof process,
      setImmediate: typeof setImmediate,
      global: typeof global,
      Buffer: typeof Buffer
    };
    console.log(JSON.stringify(types));
  } catch (e) {
    console.log(e.message);
  }
});
