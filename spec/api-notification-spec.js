const assert = require('assert')
const {remote} = require('electron')
const {Notification} = remote

describe('Notification module', function () {
  describe('Notification', function () {
    it('should show', function () {
      assert.doesNotThrow(() => {
        var notification = new Notification({
          title: 'Hello, world!',
          body: 'This is body'
        })
        notification.show()
      })
    })

    it('should close', function () {
      assert.doesNotThrow(() => {
        var notification = new Notification({
          title: 'Hello, world!',
          body: 'This is body'
        })
        notification.show()
        notification.close()
      })
    })

    it('should close all', function () {
      assert.doesNotThrow(() => {
        var notification = new Notification({
          title: 'Hello, world!',
          body: 'This is body'
        })
        notification.show()
        Notification.closeAll()
      })
    })
  })
})
