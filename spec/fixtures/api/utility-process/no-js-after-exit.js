const { writeFileSync } = require('node:fs');

const arg = process.argv[2];
const file = arg.split('=')[1];
writeFileSync(file, 'before exit');
process.exit(1);
writeFileSync(file, 'after exit');
