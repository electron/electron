function write (writable, chunk) {
  return new Promise((resolve) => writable.write(chunk, resolve));
}

write(process.stdout, 'hello\n')
  .then(() => write(process.stderr, 'world'))
  .then(() => process.exit(0));
