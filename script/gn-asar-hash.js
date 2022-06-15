const asar = require('asar');
const crypto = require('crypto');
const fs = require('fs');

const archive = process.argv[2];
const hashFile = process.argv[3];

const { headerString } = asar.getRawHeader(archive);
fs.writeFileSync(hashFile, crypto.createHash('SHA256').update(headerString).digest('hex'));
