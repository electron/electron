import * as fs from 'node:fs/promises';

const source = process.argv[2];
const destination = process.argv[3];

await fs.cp(source, destination);
