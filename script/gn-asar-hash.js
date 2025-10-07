const asar = require('@electron/asar');

const crypto = require('node:crypto');
const fs = require('node:fs');

const archive = process.argv[2];
const hashFile = process.argv[3];

const { headerString } = asar.getRawHeader(archive);
fs.writeFileSync(hashFile, crypto.createHash('SHA256').update(headerString).digest('hex'));
