process.on('uncaughtException', function(error) {
  console.error('uncaughtException:');
  if (error.stack)
    console.error(error.stack);
  else
    console.error(error.name + ': ' + error.message);
});

console.log(process.atom_binding('window'));
