Object.defineProperties exports,
  # Common modules, please sort with alphabet order.
  clipboard:
    # Must be enumerable, otherwise it woulde be invisible to remote module.
    enumerable: true
    get: -> require '../clipboard'
  crashRepoter:
    enumerable: true
    get: -> require '../crash-reporter'
  nativeImage:
    enumerable: true
    get: -> require '../native-image'
  shell:
    enumerable: true
    get: -> require '../shell'
