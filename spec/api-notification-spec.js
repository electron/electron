const chai = require('chai')
const dirtyChai = require('dirty-chai')

const { expect } = chai
chai.use(dirtyChai)

const { Notification } = require('electron').remote

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

    expect(n.title).to.equal('title')
    n.title = 'title1'
    expect(n.title).to.equal('title1')

    expect(n.subtitle).equal('subtitle')
    n.subtitle = 'subtitle1'
    expect(n.subtitle).equal('subtitle1')

    expect(n.body).to.equal('body')
    n.body = 'body1'
    expect(n.body).to.equal('body1')

    expect(n.replyPlaceholder).to.equal('replyPlaceholder')
    n.replyPlaceholder = 'replyPlaceholder1'
    expect(n.replyPlaceholder).to.equal('replyPlaceholder1')

    expect(n.sound).to.equal('sound')
    n.sound = 'sound1'
    expect(n.sound).to.equal('sound1')

    expect(n.closeButtonText).to.equal('closeButtonText')
    n.closeButtonText = 'closeButtonText1'
    expect(n.closeButtonText).to.equal('closeButtonText1')
  })

  it('inits, gets and sets basic boolean properties correctly', () => {
    const n = new Notification({
      silent: true,
      hasReply: true
    })

    expect(n.silent).to.be.true()
    n.silent = false
    expect(n.silent).to.be.false()

    expect(n.hasReply).to.be.true()
    n.hasReply = false
    expect(n.hasReply).to.be.false()
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

    expect(n.actions.length).to.equal(2)
    expect(n.actions[0].type).to.equal('button')
    expect(n.actions[0].text).to.equal('1')
    expect(n.actions[1].type).to.equal('button')
    expect(n.actions[1].text).to.equal('2')

    n.actions = [
      {
        type: 'button',
        text: '3'
      }, {
        type: 'button',
        text: '4'
      }
    ]

    expect(n.actions.length).to.equal(2)
    expect(n.actions[0].type).to.equal('button')
    expect(n.actions[0].text).to.equal('3')
    expect(n.actions[1].type).to.equal('button')
    expect(n.actions[1].text).to.equal('4')
  })

  // TODO(sethlu): Find way to test init with notification icon?
})
