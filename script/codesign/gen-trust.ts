import * as cp from 'node:child_process';
import * as fs from 'node:fs';
import * as path from 'node:path';

const certificatePath = process.argv[2];
const outPath = process.argv[3];
const templatePath = path.resolve(__dirname, 'trust.xml');

const template = fs.readFileSync(templatePath, 'utf8');

const fingerprintResult = cp.spawnSync('openssl', ['x509', '-noout', '-fingerprint', '-sha1', '-in', certificatePath]);
if (fingerprintResult.status !== 0) {
  console.error(fingerprintResult.stderr.toString());
  process.exit(1);
}

const fingerprint = fingerprintResult.stdout.toString().replace(/^SHA1 Fingerprint=/, '').replace(/:/g, '').trim();

const serialResult = cp.spawnSync('openssl', ['x509', '-serial', '-noout', '-in', certificatePath]);
if (serialResult.status !== 0) {
  console.error(serialResult.stderr.toString());
  process.exit(1);
}

let serialHex = serialResult.stdout.toString().replace(/^serial=/, '').trim();
// Pad the serial number out to 18 hex chars
while (serialHex.length < 18) {
  serialHex = `0${serialHex}`;
}
const serialB64 = Buffer.from(serialHex, 'hex').toString('base64');

const trust = template
  .replace(/{{FINGERPRINT}}/g, fingerprint)
  .replace(/{{SERIAL_BASE64}}/g, serialB64);

fs.writeFileSync(outPath, trust);

console.log('Generated Trust Settings');
