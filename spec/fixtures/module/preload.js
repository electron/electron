const types = {
  require: typeof require,
  module: typeof module,
  exports: typeof exports,
  process: typeof process,
  Buffer: typeof Buffer
};
console.log(JSON.stringify(types));
