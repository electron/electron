const TestController = require('./TestController')
require('./define-helpers')

if (process.mainModule === module) {
  (async () => {
    const controller = new TestController()
    await controller.registerTests()
    await controller.run()
  })().catch((err) => {
    console.error(err)
    process.exit(1)
  })
}