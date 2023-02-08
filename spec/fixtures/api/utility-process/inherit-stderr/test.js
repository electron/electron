process.stderr.write(process.argv[2].split('--payload=')[1]);
process.stderr.end();
process.exit(0);
