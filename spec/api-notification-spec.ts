import { Notification } from 'electron/main';

import { expect } from 'chai';

import { once } from 'node:events';

import { ifit } from './lib/spec-helpers';

describe('Notification module', () => {
  it('sets the correct class name on the prototype', () => {
    expect(Notification.prototype.constructor.name).to.equal('Notification');
  });

  it('is supported', () => {
    expect(Notification.isSupported()).to.be.a('boolean');
  });

  ifit(process.platform === 'darwin')('inits, gets and sets id property', () => {
    const n = new Notification({
      id: 'my-custom-id',
      title: 'title',
      body: 'body'
    });

    expect(n.id).to.equal('my-custom-id');
    n.id = 'new-id';
    expect(n.id).to.equal('new-id');
  });

  ifit(process.platform === 'darwin')('defaults id to empty string when not provided', () => {
    const n = new Notification({
      title: 'title',
      body: 'body'
    });

    expect(n.id).to.equal('');
  });

  ifit(process.platform === 'darwin')('inits, gets and sets groupId property', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      groupId: 'E017VKL2N8H|C07RBMNS9EK'
    });

    expect(n.groupId).to.equal('E017VKL2N8H|C07RBMNS9EK');
    n.groupId = 'new-group';
    expect(n.groupId).to.equal('new-group');
  });

  ifit(process.platform === 'darwin')('defaults groupId to empty string when not provided', () => {
    const n = new Notification({
      title: 'title',
      body: 'body'
    });

    expect(n.groupId).to.equal('');
  });

  it('inits, gets and sets basic string properties correctly', () => {
    const n = new Notification({
      title: 'title',
      subtitle: 'subtitle',
      body: 'body',
      replyPlaceholder: 'replyPlaceholder',
      sound: 'sound',
      closeButtonText: 'closeButtonText'
    });

    expect(n.title).to.equal('title');
    n.title = 'title1';
    expect(n.title).to.equal('title1');

    expect(n.subtitle).equal('subtitle');
    n.subtitle = 'subtitle1';
    expect(n.subtitle).equal('subtitle1');

    expect(n.body).to.equal('body');
    n.body = 'body1';
    expect(n.body).to.equal('body1');

    expect(n.replyPlaceholder).to.equal('replyPlaceholder');
    n.replyPlaceholder = 'replyPlaceholder1';
    expect(n.replyPlaceholder).to.equal('replyPlaceholder1');

    expect(n.sound).to.equal('sound');
    n.sound = 'sound1';
    expect(n.sound).to.equal('sound1');

    expect(n.closeButtonText).to.equal('closeButtonText');
    n.closeButtonText = 'closeButtonText1';
    expect(n.closeButtonText).to.equal('closeButtonText1');
  });

  it('inits, gets and sets basic boolean properties correctly', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      silent: true,
      hasReply: true
    });

    expect(n.silent).to.be.true('silent');
    n.silent = false;
    expect(n.silent).to.be.false('silent');

    expect(n.hasReply).to.be.true('has reply');
    n.hasReply = false;
    expect(n.hasReply).to.be.false('has reply');
  });

  it('inits, gets and sets actions correctly', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      actions: [
        {
          type: 'button',
          text: '1'
        }, {
          type: 'button',
          text: '2'
        }
      ]
    });

    expect(n.actions.length).to.equal(2);
    expect(n.actions[0].type).to.equal('button');
    expect(n.actions[0].text).to.equal('1');
    expect(n.actions[1].type).to.equal('button');
    expect(n.actions[1].text).to.equal('2');

    n.actions = [
      {
        type: 'button',
        text: '3'
      }, {
        type: 'button',
        text: '4'
      }
    ];

    expect(n.actions.length).to.equal(2);
    expect(n.actions[0].type).to.equal('button');
    expect(n.actions[0].text).to.equal('3');
    expect(n.actions[1].type).to.equal('button');
    expect(n.actions[1].text).to.equal('4');
  });

  it('can be shown and closed', () => {
    const n = new Notification({
      title: 'test notification',
      body: 'test body',
      silent: true
    });
    n.show();
    n.close();
  });

  ifit(process.platform === 'win32')('inits, gets and sets custom xml', () => {
    const n = new Notification({
      toastXml: '<xml/>'
    });

    expect(n.toastXml).to.equal('<xml/>');
  });

  ifit(process.platform === 'darwin')('emits show and close events', async () => {
    const n = new Notification({
      title: 'test notification',
      body: 'test body',
      silent: true
    });
    {
      const e = once(n, 'show');
      n.show();
      await e;
    }
    {
      const e = once(n, 'close');
      n.close();
      await e;
    }
  });

  ifit(process.platform === 'darwin')('emits show and close events with custom id', async () => {
    const n = new Notification({
      id: 'test-custom-id',
      title: 'test notification',
      body: 'test body',
      silent: true
    });
    {
      const e = once(n, 'show');
      n.show();
      await e;
    }
    {
      const e = once(n, 'close');
      n.close();
      await e;
    }
  });

  ifit(process.platform === 'darwin')('emits show and close events with custom id and groupId', async () => {
    const n = new Notification({
      id: 'E017VKL2N8H|C07RBMNS9EK|1772656675.039',
      groupId: 'E017VKL2N8H|C07RBMNS9EK',
      title: 'test notification',
      body: 'test body',
      silent: true
    });
    {
      const e = once(n, 'show');
      n.show();
      await e;
    }
    {
      const e = once(n, 'close');
      n.close();
      await e;
    }
  });

  ifit(process.platform === 'win32')('emits failed event', async () => {
    const n = new Notification({
      toastXml: 'not xml'
    });
    {
      const e = once(n, 'failed');
      n.show();
      await e;
    }
  });

  // TODO(sethlu): Find way to test init with notification icon?
});
