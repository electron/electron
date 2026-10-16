// A module exporting f(), which loads an i32 at address 70000,
// past the end of its single 64 KiB memory page.
// Where V8 uses trap-based bounds checks, the load faults with SIGSEGV
// and V8's trap handler turns it into a catchable RuntimeError.
// prettier-ignore
const bytes = new Uint8Array([
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, // magic, version
  0x01, 0x05, 0x01, 0x60, 0x00, 0x01, 0x7f, // type: () -> i32
  0x03, 0x02, 0x01, 0x00, // function 0 has type 0
  0x05, 0x03, 0x01, 0x00, 0x01, // memory: 1 page
  0x07, 0x05, 0x01, 0x01, 0x66, 0x00, 0x00, // export "f" = function 0
  0x0a, 0x0b, 0x01, 0x09, 0x00, 0x41, 0xf0, 0xa2, 0x04, 0x28, 0x02, 0x00, 0x0b // i32.load(70000)
]);
const { f } = new WebAssembly.Instance(new WebAssembly.Module(bytes)).exports;

let caught = 0;
for (let i = 0; i < 2; i++) {
  try {
    f();
  } catch (error) {
    if (error instanceof WebAssembly.RuntimeError) caught++;
  }
}

// A dump written before this point lacks the parameter,
// so the spec can tell a dump of a caught trap from the real crash below.
process.crashReporter.addExtraParameter('wasmTrapsCaught', String(caught));
process.nextTick(() => process.crash());
