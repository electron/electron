// Drops the trailing .lcovfun/.lcovmap sections that lld-link places after
// .rsrc/.reloc in a Windows coverage build. The PE resource editor used by
// spec/asar-integrity.spec.ts only accepts .reloc after .rsrc, and only
// llvm-cov reads these sections (from its own copy of the binary).

import { readFileSync, writeFileSync } from 'node:fs';

const COVERAGE_MAPPING_SECTIONS = ['.lcovfun', '.lcovmap'];

const file = process.argv[2];
const pe = readFileSync(file);
const peHeader = pe.readUInt32LE(0x3c);
const sectionCount = pe.readUInt16LE(peHeader + 6);
const optionalHeader = peHeader + 24;
const sectionTable = optionalHeader + pe.readUInt16LE(peHeader + 20);

const sections = [];
for (let i = 0; i < sectionCount; i++) {
  const header = sectionTable + 40 * i;
  sections.push({
    name: pe.toString('latin1', header, header + 8).replace(/\0+$/, ''),
    virtualSize: pe.readUInt32LE(header + 8),
    virtualAddress: pe.readUInt32LE(header + 12),
    rawSize: pe.readUInt32LE(header + 16),
    rawPointer: pe.readUInt32LE(header + 20)
  });
}

let keep = sectionCount;
while (keep > 0 && COVERAGE_MAPPING_SECTIONS.includes(sections[keep - 1].name)) keep--;
if (keep === sectionCount) {
  console.log(`No trailing coverage mapping sections in ${file}`);
  process.exit(0);
}

const kept = sections.slice(0, keep);
const dropped = sections.slice(keep);
const end = Math.min(...dropped.map((s) => s.rawPointer));
if (kept.some((s) => s.rawPointer + s.rawSize > end)) {
  throw new Error(`${file}: section data after the coverage mapping sections`);
}

const sectionAlignment = pe.readUInt32LE(optionalHeader + 32);
const last = kept[keep - 1];
pe.writeUInt16LE(keep, peHeader + 6);
// SizeOfImage
pe.writeUInt32LE(
  Math.ceil((last.virtualAddress + last.virtualSize) / sectionAlignment) * sectionAlignment,
  optionalHeader + 56
);
pe.fill(0, sectionTable + 40 * keep, sectionTable + 40 * sectionCount);
writeFileSync(file, pe.subarray(0, end));
console.log(`Dropped ${dropped.map((s) => s.name).join(', ')} from ${file}`);
