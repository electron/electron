try {
  require('some-module');
} catch (err) {
  console.error(err);
  process.exit(1);
}

process.exit(0);
