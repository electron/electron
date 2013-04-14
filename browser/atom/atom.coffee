# Don't quit on fatal error.
process.on "uncaughtException", (error) ->
  console.error "uncaughtException:"
  if error.stack?
    console.error error.stack
  else
    console.error error.name + ": " + error.message
