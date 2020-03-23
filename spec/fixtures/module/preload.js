const types = {
  require: typeof require,
  module: typeof module,
  process: typeof process,
  Buffer: typeof Buffer
};
console.log(JSON.stringify(types));
