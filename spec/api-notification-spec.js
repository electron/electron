const assert = require('assert')

const {Notification} = require('electron').remote

describe('Notification module', () => {
  it('inits, gets and sets basic string properties correctly', () => {
    const n = new Notification({
      title: 'title',
      subtitle: 'subtitle',
      body: 'body',
      replyPlaceholder: 'replyPlaceholder',
      sound: 'sound',
      closeButtonText: 'closeButtonText'
    })

    assert.equal(n.title, 'title')
    n.title = 'title1'
    assert.equal(n.title, 'title1')

    assert.equal(n.subtitle, 'subtitle')
    n.subtitle = 'subtitle1'
    assert.equal(n.subtitle, 'subtitle1')

    assert.equal(n.body, 'body')
    n.body = 'body1'
    assert.equal(n.body, 'body1')

    assert.equal(n.replyPlaceholder, 'replyPlaceholder')
    n.replyPlaceholder = 'replyPlaceholder1'
    assert.equal(n.replyPlaceholder, 'replyPlaceholder1')

    assert.equal(n.sound, 'sound')
    n.sound = 'sound1'
    assert.equal(n.sound, 'sound1')

    assert.equal(n.closeButtonText, 'closeButtonText')
    n.closeButtonText = 'closeButtonText1'
    assert.equal(n.closeButtonText, 'closeButtonText1')
  })

  it('inits, gets and sets basic boolean properties correctly', () => {
    const n = new Notification({
      silent: true,
      hasReply: true
    })

    assert.equal(n.silent, true)
    n.silent = false
    assert.equal(n.silent, false)

    assert.equal(n.hasReply, true)
    n.hasReply = false
    assert.equal(n.hasReply, false)
  })

  it('inits, gets and sets actions correctly', () => {
    const n = new Notification({
      actions: [
        {
          type: 'button',
          text: '1'
        }, {
          type: 'button',
          text: '2'
        }
      ]
    })

    assert.equal(n.actions[0].type, 'button')
    assert.equal(n.actions[0].text, '1')
    assert.equal(n.actions[1].type, 'button')
    assert.equal(n.actions[1].text, '2')

    n.actions = [
      {
        type: 'button',
        text: '3'
      }, {
        type: 'button',
        text: '4'
      }
    ]

    assert.equal(n.actions[0].type, 'button')
    assert.equal(n.actions[0].text, '3')
    assert.equal(n.actions[1].type, 'button')
    assert.equal(n.actions[1].text, '4')
  })

  // TODO(sethlu): Find way to test init with notification icon?
})
