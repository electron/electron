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

  ifit(process.platform === 'darwin')('inits and gets id property', () => {
    const n = new Notification({
      id: 'my-custom-id',
      title: 'title',
      body: 'body'
    });

    expect(n.id).to.equal('my-custom-id');
  });

  ifit(process.platform === 'darwin')('id is read-only', () => {
    const n = new Notification({
      id: 'my-custom-id',
      title: 'title',
      body: 'body'
    });

    expect(() => { (n as any).id = 'new-id'; }).to.throw();
  });

  ifit(process.platform === 'darwin')('defaults id to a UUID when not provided', () => {
    const n = new Notification({
      title: 'title',
      body: 'body'
    });

    expect(n.id).to.be.a('string').and.not.be.empty();
    expect(n.id).to.match(/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/);
  });

  ifit(process.platform === 'darwin')('defaults id to a UUID when empty string is provided', () => {
    const n = new Notification({
      id: '',
      title: 'title',
      body: 'body'
    });

    expect(n.id).to.match(/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/);
  });

  ifit(process.platform === 'darwin')('inits and gets groupId property', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      groupId: 'E017VKL2N8H|C07RBMNS9EK'
    });

    expect(n.groupId).to.equal('E017VKL2N8H|C07RBMNS9EK');
  });

  ifit(process.platform === 'darwin')('groupId is read-only', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      groupId: 'E017VKL2N8H|C07RBMNS9EK'
    });

    expect(() => { (n as any).groupId = 'new-group'; }).to.throw();
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

  describe('static methods', () => {
    ifit(process.platform === 'darwin')('getHistory returns a promise that resolves to an array', async () => {
      const result = Notification.getHistory();
      expect(result).to.be.a('promise');
      const history = await result;
      expect(history).to.be.an('array');
    });

    ifit(process.platform === 'darwin')('remove does not throw with a string argument', () => {
      expect(() => Notification.remove('nonexistent-id')).to.not.throw();
    });

    ifit(process.platform === 'darwin')('remove does not throw with an array argument', () => {
      expect(() => Notification.remove(['id-1', 'id-2'])).to.not.throw();
    });

    ifit(process.platform === 'darwin')('remove throws with no arguments', () => {
      expect(() => (Notification.remove as any)()).to.throw(/Expected a string or array of strings/);
    });

    ifit(process.platform === 'darwin')('remove throws with an invalid argument type', () => {
      expect(() => (Notification.remove as any)(123)).to.throw(/Expected a string or array of strings/);
    });

    ifit(process.platform === 'darwin')('removeAll does not throw', () => {
      expect(() => Notification.removeAll()).to.not.throw();
    });

    ifit(process.platform === 'darwin')('getHistory returns delivered notifications', async () => {
      const n = new Notification({
        id: 'history-test-id',
        title: 'history test',
        body: 'history body',
        groupId: 'history-group',
        silent: true
      });

      const shown = once(n, 'show');
      n.show();
      await shown;

      const history = await Notification.getHistory();
      // getHistory requires code-signed builds to return results;
      // skip the content assertions if Notification Center is empty.
      if (history.length > 0) {
        const found = history.find((item: any) => item.id === 'history-test-id');
        expect(found).to.not.be.undefined();
        expect(found.title).to.equal('history test');
        expect(found.body).to.equal('history body');
        expect(found.groupId).to.equal('history-group');
      }

      n.close();
    });

    ifit(process.platform === 'darwin')('remove removes a notification by id', async () => {
      const n = new Notification({
        id: 'remove-test-id',
        title: 'remove test',
        body: 'remove body',
        silent: true
      });

      const shown = once(n, 'show');
      n.show();
      await shown;

      Notification.remove('remove-test-id');

      // Give the notification center a moment to process the removal
      await new Promise(resolve => setTimeout(resolve, 100));

      const history = await Notification.getHistory();
      const found = history.find((item: any) => item.id === 'remove-test-id');
      expect(found).to.be.undefined();
    });

    ifit(process.platform === 'darwin')('remove accepts an array of ids', async () => {
      const n1 = new Notification({
        id: 'remove-array-1',
        title: 'test 1',
        body: 'body 1',
        silent: true
      });
      const n2 = new Notification({
        id: 'remove-array-2',
        title: 'test 2',
        body: 'body 2',
        silent: true
      });

      const shown1 = once(n1, 'show');
      n1.show();
      await shown1;

      const shown2 = once(n2, 'show');
      n2.show();
      await shown2;

      Notification.remove(['remove-array-1', 'remove-array-2']);

      await new Promise(resolve => setTimeout(resolve, 100));

      const history = await Notification.getHistory();
      const found1 = history.find((item: any) => item.id === 'remove-array-1');
      const found2 = history.find((item: any) => item.id === 'remove-array-2');
      expect(found1).to.be.undefined();
      expect(found2).to.be.undefined();
    });

    ifit(process.platform === 'darwin')('removeAll removes all notifications', async () => {
      const n = new Notification({
        id: 'remove-all-test',
        title: 'removeAll test',
        body: 'body',
        silent: true
      });

      const shown = once(n, 'show');
      n.show();
      await shown;

      Notification.removeAll();

      await new Promise(resolve => setTimeout(resolve, 100));

      const history = await Notification.getHistory();
      const found = history.find((item: any) => item.id === 'remove-all-test');
      expect(found).to.be.undefined();
    });
  });
});
