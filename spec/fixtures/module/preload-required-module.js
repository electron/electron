try {
  const types = {
    process: typeof process,
    setImmediate: typeof setImmediate,
    global: typeof global,
    Buffer: typeof Buffer,
    'global.Buffer': typeof global.Buffer
  };
  console.log(JSON.stringify(types));
} catch (e) {
  console.log(e.message);
}
