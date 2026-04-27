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

  ifit(process.platform === 'darwin' || process.platform === 'win32')('inits and gets id property', () => {
    const n = new Notification({
      id: 'my-custom-id',
      title: 'title',
      body: 'body'
    });

    expect(n.id).to.equal('my-custom-id');
  });

  ifit(process.platform === 'darwin' || process.platform === 'win32')('id is read-only', () => {
    const n = new Notification({
      id: 'my-custom-id',
      title: 'title',
      body: 'body'
    });

    expect(() => {
      (n as any).id = 'new-id';
    }).to.throw();
  });

  ifit(process.platform === 'darwin' || process.platform === 'win32')('defaults id to a UUID when not provided', () => {
    const n = new Notification({
      title: 'title',
      body: 'body'
    });

    expect(n.id).to.be.a('string').and.not.be.empty();
    expect(n.id).to.match(/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/);
  });

  ifit(process.platform === 'darwin' || process.platform === 'win32')(
    'defaults id to a UUID when empty string is provided',
    () => {
      const n = new Notification({
        id: '',
        title: 'title',
        body: 'body'
      });

      expect(n.id).to.match(/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/);
    }
  );

  ifit(process.platform === 'darwin' || process.platform === 'win32')('inits and gets groupId property', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      groupId: 'E017VKL2N8H|C07RBMNS9EK'
    });

    expect(n.groupId).to.equal('E017VKL2N8H|C07RBMNS9EK');
  });

  ifit(process.platform === 'darwin' || process.platform === 'win32')('groupId is read-only', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      groupId: 'E017VKL2N8H|C07RBMNS9EK'
    });

    expect(() => {
      (n as any).groupId = 'new-group';
    }).to.throw();
  });

  ifit(process.platform === 'darwin' || process.platform === 'win32')(
    'defaults groupId to empty string when not provided',
    () => {
      const n = new Notification({
        title: 'title',
        body: 'body'
      });

      expect(n.groupId).to.equal('');
    }
  );

  ifit(process.platform === 'win32')('inits and gets groupTitle property', () => {
    const n = new Notification({
      title: 'title',
      body: 'body',
      groupId: 'my-group',
      groupTitle: 'My Group Title'
    });

    expect(n.groupTitle).to.equal('My Group Title');
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
    }).to.throw();
  });

  ifit(process.platform === 'win32')('defaults groupTitle to empty string when not provided', () => {
    const n = new Notification({
      title: 'title',
      body: 'body'
    });

    expect(n.groupTitle).to.equal('');
  });

  ifit(process.platform === 'win32')('throws when id exceeds 64 characters', () => {
    expect(() => {
      // eslint-disable-next-line no-new
      new Notification({
        id: 'a'.repeat(65),
        title: 'title',
        body: 'body'
      });
    }).to.throw(/id exceeds Windows limit of 64 UTF-16 characters/);
  });

  ifit(process.platform === 'win32')('throws when groupId exceeds 64 characters', () => {
    expect(() => {
      // eslint-disable-next-line no-new
      new Notification({
        groupId: 'a'.repeat(65),
        title: 'title',
        body: 'body'
      });
    }).to.throw(/groupId exceeds Windows limit of 64 UTF-16 characters/);
  });

  ifit(process.platform === 'win32')('throws when groupTitle is set without groupId', () => {
    expect(() => {
      // eslint-disable-next-line no-new
      new Notification({
        groupTitle: 'My Group',
        title: 'title',
        body: 'body'
      });
    }).to.throw(/groupTitle requires groupId to be set/);
  });

  ifit(process.platform === 'win32')(
    'throws when id contains &, =, or control characters (Windows toast launch)',
    () => {
      expect(() => {
        // eslint-disable-next-line no-new
        new Notification({
          id: 'foo&group=evil',
          title: 'title',
          body: 'body'
        });
      }).to.throw(/id cannot contain/);

      expect(() => {
        // eslint-disable-next-line no-new
        new Notification({
          id: 'a=b',
          title: 'title',
          body: 'body'
        });
      }).to.throw(/id cannot contain/);

      expect(() => {
        // eslint-disable-next-line no-new
        new Notification({
          id: 'line\nbreak',
          title: 'title',
          body: 'body'
        });
      }).to.throw(/id cannot contain/);

      expect(() => {
        // eslint-disable-next-line no-new
        new Notification({
          id: 'x\u007fy',
          title: 'title',
          body: 'body'
        });
      }).to.throw(/id cannot contain/);
    }
  );

  ifit(process.platform === 'win32')(
    'throws when groupId contains &, =, or control characters (Windows toast launch)',
    () => {
      expect(() => {
        // eslint-disable-next-line no-new
        new Notification({
          title: 'title',
          body: 'body',
          groupId: 'g&h=i'
        });
      }).to.throw(/groupId cannot contain/);

      expect(() => {
        // eslint-disable-next-line no-new
        new Notification({
          title: 'title',
          body: 'body',
          groupId: 'group\nid'
        });
      }).to.throw(/groupId cannot contain/);
    }
  );

  ifit(process.platform === 'win32')('accepts id and groupId at 64 characters', () => {
    const n = new Notification({
      id: 'a'.repeat(64),
      groupId: 'b'.repeat(64),
      title: 'title',
      body: 'body'
    });

    expect(n.id).to.equal('a'.repeat(64));
    expect(n.groupId).to.equal('b'.repeat(64));
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
        },
        {
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
      },
      {
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
    ifit(process.platform === 'darwin' || process.platform === 'win32')(
      'getHistory returns a promise that resolves to an array',
      async () => {
        const result = Notification.getHistory();
        expect(result).to.be.a('promise');
        const history = await result;
        expect(history).to.be.an('array');
      }
    );

    ifit(process.platform === 'darwin' || process.platform === 'win32')(
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

        // On Windows, a toast is only persisted to Action Center after the
        // popup is dismissed. Close it and give the OS a moment to persist.
        if (process.platform === 'win32') {
          n.close();
          await new Promise((resolve) => setTimeout(resolve, 1000));
        }

        const history = await Notification.getHistory();

        if (process.platform === 'darwin') {
          // getHistory requires code-signed builds to return results;
          // skip the content assertions if Notification Center is empty.
          if (history.length > 0) {
            const found = history.find((item: any) => item.id === 'history-test-id');
            if (!found) {
              expect.fail('Expected to find notification with id "history-test-id" in history');
            }
            expect(found).to.be.an.instanceOf(Notification);
            expect(found.title).to.equal('history test');
            expect(found.subtitle).to.equal('history subtitle');
            expect(found.body).to.equal('history body');
            expect(found.groupId).to.equal('history-group');
          }
          n.close();
        } else {
          const found = history.find((item: any) => item.id === 'history-test-id');
          if (found) {
            expect(found).to.be.an.instanceOf(Notification);
            expect(found.title).to.equal('history test');
            expect(found.body).to.equal('history body');
            expect(found.groupId).to.equal('history-group');
          }
        }
      }
    );

    ifit(process.platform === 'darwin' || process.platform === 'win32')(
      'getHistory returned notifications can be shown and closed',
      async () => {
        const n = new Notification({
          id: 'history-show-close',
          title: 'show close test',
          body: 'body',
          silent: true
        });

        const shown = once(n, 'show');
        n.show();
        await shown;

        if (process.platform === 'win32') {
          n.close();
          await new Promise((resolve) => setTimeout(resolve, 1000));
        }

        const history = await Notification.getHistory();
        const found = history.find((item: any) => item.id === 'history-show-close');
        if (found) {
          expect(() => {
            found.show();
            found.close();
          }).to.not.throw();
        }

        if (process.platform !== 'win32') {
          n.close();
        }
      }
    );
  });
});
