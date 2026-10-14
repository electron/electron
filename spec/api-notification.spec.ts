import { Notification } from 'electron/main';

import { describe, expect, it } from 'vitest';

import { once } from 'node:events';

import { ifit } from './lib/spec-helpers.ts';

describe('Notification module', () => {
  it('sets the correct class name on the prototype', () => {
    expect(Notification.prototype.constructor.name).toBe('Notification');
  });

  it('is supported', () => {
    expect(Notification.isSupported()).toBeTypeOf('boolean');
  });

  ifit(process.platform === 'darwin' || process.platform === 'win32')('inits and gets id property', () => {
    const n = new Notification({
      id: 'my-custom-id',
      title: 'title',
      body: 'body'
    });

    expect(n.id).toBe('my-custom-id');
  });

  ifit(process.platform === 'darwin' || process.platform === 'win32')('id is read-only', () => {
    const n = new Notification({
      id: 'my-custom-id',
      title: 'title',
      body: 'body'
    });

    expect(() => {
      (n as any).id = 'new-id';
    }).toThrow();
  });

  ifit(process.platform === 'darwin' || process.platform === 'win32')('defaults id to a UUID when not provided', () => {
    const n = new Notification({
      title: 'title',
      body: 'body'
    });

    expect(n.id).toBeTypeOf('string');
    expect(n.id).not.toHaveLength(0);
    expect(n.id).toMatch(/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/);
  });

  ifit(process.platform === 'darwin' || process.platform === 'win32')(
    'defaults id to a UUID when empty string is provided',
    () => {
      const n = new Notification({
        id: '',
        title: 'title',
        body: 'body'
      });

      expect(n.id).toMatch(/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/);
    }
  );

  ifit(process.platform === 'darwin' || process.platform === 'win32')('inits and gets groupId property', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      groupId: 'E017VKL2N8H|C07RBMNS9EK'
    });

    expect(n.groupId).toBe('E017VKL2N8H|C07RBMNS9EK');
  });

  ifit(process.platform === 'darwin' || process.platform === 'win32')('groupId is read-only', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      groupId: 'E017VKL2N8H|C07RBMNS9EK'
    });

    expect(() => {
      (n as any).groupId = 'new-group';
    }).toThrow();
  });

  ifit(process.platform === 'darwin' || process.platform === 'win32')(
    'defaults groupId to empty string when not provided',
    () => {
      const n = new Notification({
        title: 'title',
        body: 'body'
      });

      expect(n.groupId).toBe('');
    }
  );

  ifit(process.platform === 'win32')('inits and gets groupTitle property', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      groupId: 'my-group',
      groupTitle: 'My Group Title'
    });

    expect(n.groupTitle).toBe('My Group Title');
  });

  ifit(process.platform === 'win32')('groupTitle is read-only', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      groupId: 'my-group',
      groupTitle: 'My Group Title'
    });

    expect(() => {
      (n as any).groupTitle = 'new-title';
    }).toThrow();
  });

  ifit(process.platform === 'win32')('defaults groupTitle to empty string when not provided', () => {
    const n = new Notification({
      title: 'title',
      body: 'body'
    });

    expect(n.groupTitle).toBe('');
  });

  ifit(process.platform === 'win32')('throws when id exceeds 64 characters', () => {
    expect(() => {
      // eslint-disable-next-line no-new
      new Notification({
        id: 'a'.repeat(65),
        title: 'title',
        body: 'body'
      });
    }).toThrow(/id exceeds Windows limit of 64 UTF-16 characters/);
  });

  ifit(process.platform === 'win32')('throws when groupId exceeds 64 characters', () => {
    expect(() => {
      // eslint-disable-next-line no-new
      new Notification({
        groupId: 'a'.repeat(65),
        title: 'title',
        body: 'body'
      });
    }).toThrow(/groupId exceeds Windows limit of 64 UTF-16 characters/);
  });

  ifit(process.platform === 'win32')('throws when groupTitle is set without groupId', () => {
    expect(() => {
      // eslint-disable-next-line no-new
      new Notification({
        groupTitle: 'My Group',
        title: 'title',
        body: 'body'
      });
    }).toThrow(/groupTitle requires groupId to be set/);
  });

  ifit(process.platform === 'win32')('accepts id and groupId at 64 characters', () => {
    const n = new Notification({
      id: 'a'.repeat(64),
      groupId: 'b'.repeat(64),
      title: 'title',
      body: 'body'
    });

    expect(n.id).toBe('a'.repeat(64));
    expect(n.groupId).toBe('b'.repeat(64));
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

    expect(n.title).toBe('title');
    n.title = 'title1';
    expect(n.title).toBe('title1');

    expect(n.subtitle).toBe('subtitle');
    n.subtitle = 'subtitle1';
    expect(n.subtitle).toBe('subtitle1');

    expect(n.body).toBe('body');
    n.body = 'body1';
    expect(n.body).toBe('body1');

    expect(n.replyPlaceholder).toBe('replyPlaceholder');
    n.replyPlaceholder = 'replyPlaceholder1';
    expect(n.replyPlaceholder).toBe('replyPlaceholder1');

    expect(n.sound).toBe('sound');
    n.sound = 'sound1';
    expect(n.sound).toBe('sound1');

    expect(n.closeButtonText).toBe('closeButtonText');
    n.closeButtonText = 'closeButtonText1';
    expect(n.closeButtonText).toBe('closeButtonText1');
  });

  it('inits, gets and sets basic boolean properties correctly', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      silent: true,
      hasReply: true
    });

    expect(n.silent, 'silent').toBe(true);
    n.silent = false;
    expect(n.silent, 'silent').toBe(false);

    expect(n.hasReply, 'has reply').toBe(true);
    n.hasReply = false;
    expect(n.hasReply, 'has reply').toBe(false);
  });

  it('inits, gets and sets actions correctly', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      actions: [
        {
          type: 'button',
          text: '1'
        },
        {
          type: 'button',
          text: '2'
        }
      ]
    });

    expect(n.actions.length).toBe(2);
    expect(n.actions[0].type).toBe('button');
    expect(n.actions[0].text).toBe('1');
    expect(n.actions[1].type).toBe('button');
    expect(n.actions[1].text).toBe('2');

    n.actions = [
      {
        type: 'button',
        text: '3'
      },
      {
        type: 'button',
        text: '4'
      }
    ];

    expect(n.actions.length).toBe(2);
    expect(n.actions[0].type).toBe('button');
    expect(n.actions[0].text).toBe('3');
    expect(n.actions[1].type).toBe('button');
    expect(n.actions[1].text).toBe('4');
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

    expect(n.toastXml).toBe('<xml/>');
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

  ifit(process.platform === 'win32')('can show notification with custom id and groupId', () => {
    const n = new Notification({
      id: 'test-custom-id',
      groupId: 'test-group',
      title: 'test notification',
      body: 'test body',
      silent: true
    });
    n.show();
    n.close();
  });

  ifit(process.platform === 'win32')('can show notification with groupId and groupTitle', () => {
    const n = new Notification({
      id: 'test-custom-id',
      groupId: 'test-group',
      groupTitle: 'Test Group Header',
      title: 'test notification',
      body: 'test body',
      silent: true
    });
    n.show();
    n.close();
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
      expect(result).toBeInstanceOf(Promise);
      const history = await result;
      expect(history).toBeInstanceOf(Array);
    });

    ifit(process.platform === 'darwin')(
      'getHistory returns Notification instances with correct properties',
      async () => {
        const n = new Notification({
          id: 'history-test-id',
          title: 'history test',
          subtitle: 'history subtitle',
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
          if (!found) {
            expect.fail('Expected to find notification with id "history-test-id" in history');
          }
          expect(found).toBeInstanceOf(Notification);
          expect(found.title).toBe('history test');
          expect(found.subtitle).toBe('history subtitle');
          expect(found.body).toBe('history body');
          expect(found.groupId).toBe('history-group');
        }

        n.close();
      }
    );

    ifit(process.platform === 'darwin')('remove does not throw with a string argument', () => {
      expect(() => Notification.remove('nonexistent-id')).not.toThrow();
    });

    ifit(process.platform === 'darwin')('remove does not throw with an array argument', () => {
      expect(() => Notification.remove(['id-1', 'id-2'])).not.toThrow();
    });

    ifit(process.platform === 'darwin')('remove throws with no arguments', () => {
      expect(() => (Notification.remove as any)()).toThrow(/Expected a string or array of strings/);
    });

    ifit(process.platform === 'darwin')('remove throws with an invalid argument type', () => {
      expect(() => (Notification.remove as any)(123)).toThrow(/Expected a string or array of strings/);
    });

    ifit(process.platform === 'darwin')('remove does not throw with an empty array', () => {
      expect(() => Notification.remove([])).not.toThrow();
    });

    ifit(process.platform === 'darwin')('remove does not throw with an empty string', () => {
      expect(() => Notification.remove('')).not.toThrow();
    });

    ifit(process.platform === 'darwin')('removeAll does not throw', () => {
      expect(() => Notification.removeAll()).not.toThrow();
    });

    ifit(process.platform === 'darwin')('removeGroup does not throw', () => {
      expect(() => Notification.removeGroup('nonexistent-group')).not.toThrow();
    });

    ifit(process.platform === 'darwin')('getHistory returned notifications can be shown and closed', async () => {
      const n = new Notification({
        id: 'history-show-close',
        title: 'show close test',
        body: 'body',
        silent: true
      });

      const shown = once(n, 'show');
      n.show();
      await shown;

      const history = await Notification.getHistory();
      if (history.length > 0) {
        const found = history.find((item: any) => item.id === 'history-show-close');
        if (!found) {
          expect.fail('Expected to find notification with id "history-show-close" in history');
        }
        // Calling show() and close() on a restored notification should not throw
        expect(() => {
          found.show();
          found.close();
        }).not.toThrow();
      }
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
      await new Promise((resolve) => setTimeout(resolve, 100));

      const history = await Notification.getHistory();
      const found = history.find((item: any) => item.id === 'remove-test-id');
      expect(found).toBeUndefined();
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

      await new Promise((resolve) => setTimeout(resolve, 100));

      const history = await Notification.getHistory();
      const found1 = history.find((item: any) => item.id === 'remove-array-1');
      const found2 = history.find((item: any) => item.id === 'remove-array-2');
      expect(found1).toBeUndefined();
      expect(found2).toBeUndefined();
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

      await new Promise((resolve) => setTimeout(resolve, 100));

      const history = await Notification.getHistory();
      const found = history.find((item: any) => item.id === 'remove-all-test');
      expect(found).toBeUndefined();
    });

    ifit(process.platform === 'darwin')('removeGroup removes notifications by groupId', async () => {
      const n1 = new Notification({
        id: 'group-keep',
        title: 'keep',
        body: 'body',
        groupId: 'group-a',
        silent: true
      });
      const n2 = new Notification({
        id: 'group-remove-1',
        title: 'remove 1',
        body: 'body',
        groupId: 'group-b',
        silent: true
      });
      const n3 = new Notification({
        id: 'group-remove-2',
        title: 'remove 2',
        body: 'body',
        groupId: 'group-b',
        silent: true
      });

      for (const n of [n1, n2, n3]) {
        const shown = once(n, 'show');
        n.show();
        await shown;
      }

      Notification.removeGroup('group-b');

      // Give the notification center a moment to fetch and remove
      await new Promise((resolve) => setTimeout(resolve, 500));

      const history = await Notification.getHistory();
      // In code-signed builds, group-a notification should remain
      // while group-b notifications should be gone
      const foundB1 = history.find((item: any) => item.id === 'group-remove-1');
      const foundB2 = history.find((item: any) => item.id === 'group-remove-2');
      expect(foundB1).toBeUndefined();
      expect(foundB2).toBeUndefined();

      // Clean up
      Notification.removeAll();
    });
  });
});
