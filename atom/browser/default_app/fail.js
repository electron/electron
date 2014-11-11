function fail() {
  throw new Error("This is an error message.");
}

function second() {
  fail()
}

function first() {
  second()
}

require('remote').getCurrentWindow().openDevTools()
window.addEventListener('click', function() {
  console.log("Below is the stack trace for a caught error.\nWhen you expand it the calls are NOT cickable.\n");
  try {
    first();
  }
  catch(error) {
    console.error(error.stack);
  }

  console.log("Below is the stack trace for an uncaught error.\nWhen you expand it the calls ARE cickable.\n");
  first();
});
