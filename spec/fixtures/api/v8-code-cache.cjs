const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const vm = require('node:vm');

const testing = process._linkedBinding('electron_common_testing');
const embedder = process.versions.v8.match(
  /^\d+\.\d+\.\d+(?:\.\d+)?(-electron\.[0-9a-f]{16})(?: \(candidate\))?$/
)?.[1];
assert.ok(embedder, `V8 is missing its patch fingerprint: ${process.versions.v8}`);
const foreignEmbedder =
  embedder === '-electron.0000000000000000' ? '-electron.1111111111111111' : '-electron.0000000000000000';
const versionHash = testing.computeV8VersionHashForTesting(embedder);
const foreignVersionHash = testing.computeV8VersionHashForTesting(foreignEmbedder);
assert.notEqual(versionHash, foreignVersionHash);

// SerializedCodeData::kVersionHashOffset (v8/src/snapshot/code-serializer.h),
// after the magic number, payload length and trusted payload length.
const kScriptVersionHashOffset = 12;
// WasmSerializer::kVersionHashOffset (v8/src/wasm/wasm-serialization.h).
const kWasmVersionHashOffset = 4;

const readVersionHash = (cache, offset) =>
  os.endianness() === 'LE' ? cache.readUInt32LE(offset) : cache.readUInt32BE(offset);
const foreignCache = (cache, offset) => {
  const copy = Buffer.from(cache);
  if (os.endianness() === 'LE') copy.writeUInt32LE(foreignVersionHash, offset);
  else copy.writeUInt32BE(foreignVersionHash, offset);
  return copy;
};

const report = (matching, foreign, rebuilt) => ({
  matchingAccepted: matching.accepted,
  matchingValue: matching.value,
  foreignAccepted: foreign.accepted,
  foreignValue: foreign.value,
  rebuiltAccepted: rebuilt.accepted,
  rebuiltValue: rebuilt.value
});

exports.javascript = () => {
  const source = '(() => 42)()';
  const producer = new vm.Script(source, { filename: 'producer.js' });
  const cache = producer.createCachedData();
  assert.equal(readVersionHash(cache, kScriptVersionHashOffset), versionHash);

  const consume = (cachedData, filename) => {
    const script = new vm.Script(source, { filename, cachedData });
    return {
      accepted: !script.cachedDataRejected,
      value: script.runInNewContext(),
      cache: script.createCachedData()
    };
  };
  const matching = consume(cache, 'matching.js');
  const foreign = consume(foreignCache(cache, kScriptVersionHashOffset), 'foreign.js');
  assert.equal(readVersionHash(foreign.cache, kScriptVersionHashOffset), versionHash);
  const rebuilt = consume(foreign.cache, 'rebuilt.js');
  return report(matching, foreign, rebuilt);
};

exports.webassembly = async (builderPath) => {
  const builderSource = fs.readFileSync(builderPath, 'utf8');
  const wireBytes = vm.runInNewContext(
    `${builderSource}
    ;(() => {
      const builder = new WasmModuleBuilder();
      builder.addMemory(1);
      builder.addFunction('grow', makeSig([kWasmI32], [kWasmI32]))
        .addBody([kExprLocalGet, 0, kExprMemoryGrow, 0])
        .exportFunc();
      return builder.toBuffer();
    })()`,
    {},
    { filename: builderPath }
  );
  const module = new WebAssembly.Module(wireBytes);
  const cache = testing.serializeWasmModuleForTesting(module);
  assert.ok(cache.length > 32, 'Expected serialized optimized Wasm code');
  assert.equal(readVersionHash(cache, kWasmVersionHashOffset), versionHash);

  const consume = async (cachedData) => {
    const result = testing.compileWasmModuleWithCacheForTesting(wireBytes, cachedData);
    const module = await result.module;
    const instance = new WebAssembly.Instance(module);
    const value = instance.exports.grow(1);
    assert.equal(instance.exports.grow(0), 2);
    return {
      accepted: result.accepted,
      value,
      cache: testing.serializeWasmModuleForTesting(module)
    };
  };
  const matching = await consume(cache);
  const foreign = await consume(foreignCache(cache, kWasmVersionHashOffset));
  assert.equal(readVersionHash(foreign.cache, kWasmVersionHashOffset), versionHash);
  const rebuilt = await consume(foreign.cache);
  return report(matching, foreign, rebuilt);
};
