import { BrowserWindow, session, utilityProcess } from 'electron/main';

import { expect } from 'chai';

import { on, once } from 'node:events';
import * as path from 'node:path';

import { ifdescribe } from './lib/spec-helpers';
import { closeAllWindows } from './lib/window-helpers';

const features = process._linkedBinding('electron_common_features');

function getFixturePath(fixtureName: string) {
  return path.join(path.resolve(__dirname, 'fixtures', 'api', 'local-ai-handler'), fixtureName);
}

// Await fn and listen for a message of the given type, returning the message once received
// Used to listen for a message triggered as a side effect of fn, where we don't care about the result of fn
async function listenForMessage(
  aiHandler: Electron.UtilityProcess,
  messageType: string,
  fn: () => Promise<void> | void
) {
  const messages = on(aiHandler, 'message');
  await fn();

  for await (const [message] of messages) {
    if (message.type === messageType) {
      return message;
    }
  }

  return null;
}

// Call fn and await a message of the given type, returning the message and the promise returned by fn
// Used to listen for a message triggered as a side effect of fn, where we do care about the result of fn
async function waitForMessage(aiHandler: Electron.UtilityProcess, messageType: string, fn: () => Promise<unknown>) {
  let promise: Promise<unknown>;

  await listenForMessage(aiHandler, messageType, () => {
    promise = fn();
  });

  return { promise: promise! };
}

ifdescribe(features.isPromptAPIEnabled())('localAIHandler module', () => {
  const fixtures = path.resolve(__dirname, 'fixtures');

  let w: Electron.BrowserWindow;

  async function forkAndRegisterHandler(fixtureName: string) {
    const aiHandler = utilityProcess.fork(getFixturePath(fixtureName));
    await once(aiHandler, 'spawn');
    w.webContents.session.registerLocalAIHandler(aiHandler);

    return aiHandler;
  }

  async function sendControllableMessage(aiHandler: Electron.UtilityProcess, message: unknown) {
    const ackEvent = once(aiHandler, 'message');
    aiHandler.postMessage(message);
    await ackEvent;
  }

  beforeEach(async () => {
    w = new BrowserWindow({
      show: false,
      webPreferences: {
        enableBlinkFeatures: 'AIPromptAPI,AIPromptAPIMultimodalInput'
      }
    });

    await w.loadFile(path.join(fixtures, 'api', 'blank.html'));
  });

  afterEach(() => {
    w.webContents.session.registerLocalAIHandler(null);
    closeAllWindows();
  });

  describe('LanguageModel.availability()', () => {
    it('is unavailable if invalid value returned', async () => {
      await forkAndRegisterHandler('buggy-language-model.js');

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('unavailable');
    });

    it('returns "available" when handler reports available', async () => {
      await forkAndRegisterHandler('default-language-model.js');

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('available');
    });

    it('returns "available" when late registered handler reports available', async () => {
      const aiHandler = await forkAndRegisterHandler('delayed-prompt-handler.js');

      await listenForMessage(aiHandler, 'in-progress-prompt-handler-call', async () => {
        await w.webContents.executeJavaScript('const availability = LanguageModel.availability()');
      });
      await sendControllableMessage(aiHandler, { command: 'set-handler' });

      expect(await w.webContents.executeJavaScript('availability')).to.equal('available');
    });

    it('returns "downloadable" when handler reports downloadable', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-availability', value: 'downloadable' });

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('downloadable');
    });

    it('returns "downloading" when handler reports downloading', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-availability', value: 'downloading' });

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('downloading');
    });

    it('returns "unavailable" when handler reports unavailable', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-availability', value: 'unavailable' });

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('unavailable');
    });

    it('returns "unavailable" when the availability() promise rejects', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-availability', value: 'reject' });

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('unavailable');
    });

    it('returns "unavailable" if the utility process dies', async () => {
      const aiHandler = await forkAndRegisterHandler('default-language-model.js');

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('available');
      aiHandler.kill();
      await once(aiHandler, 'exit');

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('unavailable');
    });

    it('returns "unavailable" if the utility process dies without ever setting a handler', async () => {
      const aiHandler = await forkAndRegisterHandler('delayed-prompt-handler.js');

      await listenForMessage(aiHandler, 'in-progress-prompt-handler-call', async () => {
        await w.webContents.executeJavaScript('const availability = LanguageModel.availability()');
      });

      aiHandler.kill();
      await once(aiHandler, 'exit');

      expect(await w.webContents.executeJavaScript('availability')).to.equal('unavailable');
    });

    it('returns "unavailable" if not registered', async () => {
      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('unavailable');
    });

    it('passes options to the availability() call', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-availability', value: 'downloading' });

      const options = {
        expectedInputs: [{ type: 'image' }, { type: 'text', languages: ['en', 'fr'] }],
        expectedOutputs: [{ type: 'image' }, { type: 'text', languages: ['en', 'fr'] }]
      };

      const message = once(aiHandler, 'message');
      await w.webContents.executeJavaScript(`LanguageModel.availability(${JSON.stringify(options)})`);
      const [receivedMessage] = await message;

      expect(receivedMessage.options).to.deep.equal(options);
      expect(receivedMessage.type).to.equal('availability-called');
    });
  });

  describe('LanguageModel.create()', () => {
    async function expectRejectedWithError(message: string | RegExp, options?: Object) {
      // Unwrap the error message because NotAllowedError won't serialize
      if (options) {
        await expect(
          w.webContents.executeJavaScript(
            `LanguageModel.create(${JSON.stringify(options)}).catch(err => { throw err.message; })`
          )
        ).to.eventually.be.rejectedWith(message);
      } else {
        await expect(
          w.webContents.executeJavaScript('LanguageModel.create().catch(err => { throw err.message; })')
        ).to.eventually.be.rejectedWith(message);
      }
    }

    it('rejects if invalid value returned', async () => {
      await forkAndRegisterHandler('buggy-language-model.js');

      await expectRejectedWithError(/unable to create/);
    });

    it('rejects if null returned', async () => {
      await forkAndRegisterHandler('return-null-handler.js');

      await expectRejectedWithError(/unable to create/);
    });

    it('rejects when no handler is registered', async () => {
      await expectRejectedWithError(/unable to create/);
    });

    it('rejects when handler promise rejects', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-create', value: 'reject' });

      await expectRejectedWithError(/unable to create/);
    });

    it('rejects if the utility process dies during creation', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-create', value: 'wait-for-abort' });

      const { promise } = await waitForMessage(aiHandler, 'create-called', () => {
        return w.webContents.executeJavaScript('LanguageModel.create().catch(err => { throw err.message; })');
      });

      aiHandler.kill();
      await once(aiHandler, 'exit');

      await expect(promise).to.eventually.be.rejectedWith(/unable to create/);
    });

    it('rejects if the utility process dies without ever setting a handler', async () => {
      const aiHandler = await forkAndRegisterHandler('delayed-prompt-handler.js');

      await listenForMessage(aiHandler, 'in-progress-prompt-handler-call', async () => {
        await w.webContents.executeJavaScript(
          'const pending = LanguageModel.create().then(model => model instanceof LanguageModel).catch(err => { throw err.message; })'
        );
      });

      aiHandler.kill();
      await once(aiHandler, 'exit');

      await expect(w.webContents.executeJavaScript('pending')).to.eventually.be.rejectedWith(/unable? to create/);
    });

    it('rejects if the handler gets unregistered during creation', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-create', value: 'wait-for-abort' });

      const { promise } = await waitForMessage(aiHandler, 'create-called', () => {
        return w.webContents.executeJavaScript('LanguageModel.create().catch(err => { throw err.message; })');
      });

      w.webContents.session.registerLocalAIHandler(null);

      await expect(promise).to.eventually.be.rejectedWith(/unable to create/);
    });

    it('creates a LanguageModel instance from a valid handler', async () => {
      await forkAndRegisterHandler('default-language-model.js');

      expect(
        await w.webContents.executeJavaScript('LanguageModel.create().then(model => model instanceof LanguageModel)')
      ).to.equal(true);
    });

    it('creates a LanguageModel instance from a valid handler registered late', async () => {
      const aiHandler = await forkAndRegisterHandler('delayed-prompt-handler.js');

      await listenForMessage(aiHandler, 'in-progress-prompt-handler-call', async () => {
        await w.webContents.executeJavaScript(
          'const pending = LanguageModel.create().then(model => model instanceof LanguageModel).catch(err => { throw err.message; })'
        );
      });
      await sendControllableMessage(aiHandler, { command: 'set-handler' });

      expect(await w.webContents.executeJavaScript('pending')).to.equal(true);
    });

    it('evicts the oldest pending binding when the max queue size is exceeded', async () => {
      const aiHandler = await forkAndRegisterHandler('delayed-prompt-handler.js');

      const createWindow = async () => {
        const win = new BrowserWindow({
          show: false,
          webPreferences: {
            enableBlinkFeatures: 'AIPromptAPI,AIPromptAPIMultimodalInput'
          }
        });
        await win.loadFile(path.join(fixtures, 'api', 'blank.html'));
        return win;
      };

      // Create 11 windows, each calling LanguageModel.create() before the handler is set.
      // The pending queue has a max size of 10, so the 11th evicts the 1st.
      const windows = [];
      for (let i = 0; i < 11; i++) {
        const win = i === 0 ? w : await createWindow();
        windows.push(win);
        await listenForMessage(aiHandler, 'in-progress-prompt-handler-call', async () => {
          await win.webContents.executeJavaScript(
            'const success = LanguageModel.create().then(model => model instanceof LanguageModel).catch(err => { throw err.message; })'
          );
        });
      }

      // The first window's create() should have been rejected (evicted from queue).
      await expect(windows[0].webContents.executeJavaScript('success')).to.eventually.be.rejectedWith(
        /unable to create/
      );

      // Set the handler so remaining pending bindings get flushed.
      await sendControllableMessage(aiHandler, { command: 'set-handler' });

      // The remaining windows should now succeed.
      for (let i = 1; i < 11; i++) {
        expect(await windows[i].webContents.executeJavaScript('success')).to.equal(true);
      }
    });

    it('passes initialPrompts to create()', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');

      const options = {
        initialPrompts: [{ role: 'system', content: [{ type: 'text', value: 'You are Electron AI' }] }]
      };

      const message = await listenForMessage(aiHandler, 'create-called', async () => {
        await w.webContents.executeJavaScript(`LanguageModel.create(${JSON.stringify(options)})`);
      });

      expect(message.options).to.have.property('signal');
      delete message.options.signal;
      expect(message.options).to.deep.equal({
        initialPrompts: options.initialPrompts.map((prompt) => ({ ...prompt, prefix: false }))
      });
    });

    it('passes expectedInputs and expectedOutputs options', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');

      const options = {
        expectedInputs: [{ type: 'image' }, { type: 'text', languages: ['en', 'fr'] }],
        expectedOutputs: [{ type: 'text', languages: ['en', 'fr'] }]
      };

      const message = await listenForMessage(aiHandler, 'create-called', async () => {
        await w.webContents.executeJavaScript(`LanguageModel.create(${JSON.stringify(options)})`);
      });

      expect(message.options).to.have.property('signal');
      delete message.options.signal;
      expect(message.options).to.deep.equal(options);
    });

    it('plumbs the abort signal through', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-create', value: 'wait-for-abort' });

      const message = await listenForMessage(aiHandler, 'create-aborted', async () => {
        await expect(
          w.webContents.executeJavaScript(
            'LanguageModel.create({ signal: AbortSignal.timeout(500) }).catch(err => { throw err.message; })'
          )
        ).to.eventually.be.rejectedWith(/signal timed out/);
      });

      expect(message).not.null();
    });

    it('exposes contextUsage and contextWindow on the created model', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      expect(
        await w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => ({ contextUsage: model.contextUsage, contextWindow: model.contextWindow }))'
        )
      ).to.deep.equal({ contextUsage: 0, contextWindow: 12345 });
    });
  });

  describe('LanguageModel.prompt()', () => {
    async function expectRejectedWithError(message: string | RegExp, prompt: string, options?: Object) {
      // Unwrap the error message because NotAllowedError won't serialize
      if (options) {
        await expect(
          w.webContents.executeJavaScript(
            `LanguageModel.create().then(model => model.prompt(${JSON.stringify(prompt)}, ${JSON.stringify(options)})).catch(err => { throw err.message; })`
          )
        ).to.eventually.be.rejectedWith(message);
      } else {
        await expect(
          w.webContents.executeJavaScript(
            `LanguageModel.create().then(model => model.prompt(${JSON.stringify(prompt)})).catch(err => { throw err.message; })`
          )
        ).to.eventually.be.rejectedWith(message);
      }
    }

    it('rejects when handler returns an invalid value', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-prompt-response', value: 99 });

      await expectRejectedWithError(/error occurred/, 'Test prompt');
    });

    it('rejects when handler promise rejects', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-prompt-response', value: 'reject' });

      await expectRejectedWithError(/error occurred/, 'Test prompt');
    });

    it('rejects after the model has been destroyed', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      await expect(
        w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => { model.destroy(); return model.prompt("Test") }).catch(err => { throw err.message; })'
        )
      ).to.eventually.be.rejectedWith(/has been destroyed/);
    });

    it('rejects if the utility process dies during prompt', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-prompt-response', value: 'wait-for-abort' });

      const { promise } = await waitForMessage(aiHandler, 'prompt-called', () => {
        return w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.prompt("Test")).catch(err => { throw err.message; })'
        );
      });

      aiHandler.kill();
      await once(aiHandler, 'exit');

      await expect(promise).to.eventually.be.rejectedWith(/has been destroyed/);
    });

    it('rejects if the handler gets unregistered during prompt', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-prompt-response', value: 'wait-for-abort' });

      const { promise } = await waitForMessage(aiHandler, 'prompt-called', () => {
        return w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.prompt("Test")).catch(err => { throw err.message; })'
        );
      });

      w.webContents.session.registerLocalAIHandler(null);

      await expect(promise).to.eventually.be.rejectedWith(/has been destroyed/);
    });

    it('returns a string response from the handler', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      expect(
        await w.webContents.executeJavaScript('LanguageModel.create().then(model => model.prompt("Hi"))')
      ).to.equal('foobar');
    });

    it('returns a ReadableStream response from the handler', async () => {
      await forkAndRegisterHandler('streaming-language-model.js');

      expect(
        await w.webContents.executeJavaScript('LanguageModel.create().then(model => model.prompt("Hi"))')
      ).to.equal('Hello World');
    });

    it('passes string input to the handler', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');

      const message = await listenForMessage(aiHandler, 'prompt-called', async () => {
        await w.webContents.executeJavaScript("LanguageModel.create().then(model => model.prompt('hello world'))");
      });

      expect(message.input).to.deep.equal([
        { role: 'user', content: [{ type: 'text', value: 'hello world' }], prefix: false }
      ]);
    });

    it('passes LanguageModelMessage[] input to the handler', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');

      const input = [
        { role: 'user', content: [{ type: 'text', value: 'hello' }] },
        { role: 'assistant', content: [{ type: 'text', value: 'hi' }] }
      ];

      const message = await listenForMessage(aiHandler, 'prompt-called', async () => {
        await w.webContents.executeJavaScript(
          `LanguageModel.create().then(model => model.prompt(${JSON.stringify(input)}))`
        );
      });

      expect(message.input).to.deep.equal(input.map((msg) => ({ ...msg, prefix: false })));
    });

    it('passes responseConstraint option to the handler', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');

      const responseConstraint = { type: 'object', properties: { name: { type: 'string' } } };

      const message = await listenForMessage(aiHandler, 'prompt-called', async () => {
        await w.webContents.executeJavaScript(
          `LanguageModel.create().then(model => model.prompt('test', { responseConstraint: ${JSON.stringify(responseConstraint)} }))`
        );
      });

      expect(message.options.responseConstraint).to.deep.equal(responseConstraint);
    });

    it('plumbs the abort signal through', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-prompt-response', value: 'wait-for-abort' });

      const message = await listenForMessage(aiHandler, 'prompt-aborted', async () => {
        await expect(
          w.webContents.executeJavaScript(
            'LanguageModel.create().then(model => model.prompt("test", { signal: AbortSignal.timeout(500) })).catch(err => { throw err.message; })'
          )
        ).to.eventually.be.rejectedWith(/signal timed out/);
      });

      expect(message).not.null();
    });

    it('updates contextUsage after a prompt', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      expect(
        await w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => ({ contextUsage: model.contextUsage }))'
        )
      ).to.deep.equal({ contextUsage: 0 });
      expect(
        await w.webContents.executeJavaScript(
          'LanguageModel.create().then(async (model) => { await model.prompt("hello world"); return { contextUsage: model.contextUsage } })'
        )
      ).to.deep.equal({ contextUsage: 10 });
    });
  });

  describe('LanguageModel.promptStreaming()', () => {
    const collectStream =
      'async (stream) => { const reader = stream.getReader(); let r = ""; while (true) { const { done, value } = await reader.read(); if (done) return r; r += value; } }';

    async function expectRejectedWithError(message: string | RegExp, prompt: string, options?: Object) {
      const collectStreamFn = collectStream;
      // Unwrap the error message because NotAllowedError won't serialize
      if (options) {
        await expect(
          w.webContents.executeJavaScript(
            `LanguageModel.create().then(async (model) => { const collect = ${collectStreamFn}; return collect(model.promptStreaming(${JSON.stringify(prompt)}, ${JSON.stringify(options)})); }).catch(err => { throw err.message; })`
          )
        ).to.eventually.be.rejectedWith(message);
      } else {
        await expect(
          w.webContents.executeJavaScript(
            `LanguageModel.create().then(async (model) => { const collect = ${collectStreamFn}; return collect(model.promptStreaming(${JSON.stringify(prompt)})); }).catch(err => { throw err.message; })`
          )
        ).to.eventually.be.rejectedWith(message);
      }
    }

    it('rejects when handler returns an invalid value', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-prompt-response', value: 99 });

      await expectRejectedWithError(/error occurred/, 'Test prompt');
    });

    it('rejects when ReadableStream returns an invalid value', async () => {
      await forkAndRegisterHandler('buggy-streaming-language-model.js');

      await expectRejectedWithError(/has been destroyed/, 'Test prompt');
    });

    it('rejects when handler promise rejects', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-prompt-response', value: 'reject' });

      await expectRejectedWithError(/error occurred/, 'Test prompt');
    });

    it('rejects after the model has been destroyed', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      await expect(
        w.webContents.executeJavaScript(
          `LanguageModel.create().then(async (model) => { model.destroy(); const collect = ${collectStream}; return collect(model.promptStreaming("Test")); }).catch(err => { throw err.message; })`
        )
      ).to.eventually.be.rejectedWith(/has been destroyed/);
    });

    it('rejects if the utility process dies during prompt', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-prompt-response', value: 'wait-for-abort' });

      const { promise } = await waitForMessage(aiHandler, 'prompt-called', () => {
        return w.webContents.executeJavaScript(
          `LanguageModel.create().then(async (model) => { const collect = ${collectStream}; return collect(model.promptStreaming("Test")); }).catch(err => { throw err.message; })`
        );
      });

      aiHandler.kill();
      await once(aiHandler, 'exit');

      await expect(promise).to.eventually.be.rejectedWith(/has been destroyed/);
    });

    it('rejects if the handler gets unregistered during prompt', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-prompt-response', value: 'wait-for-abort' });

      const { promise } = await waitForMessage(aiHandler, 'prompt-called', () => {
        return w.webContents.executeJavaScript(
          `LanguageModel.create().then(async (model) => { const collect = ${collectStream}; return collect(model.promptStreaming("Test")); }).catch(err => { throw err.message; })`
        );
      });

      w.webContents.session.registerLocalAIHandler(null);

      await expect(promise).to.eventually.be.rejectedWith(/has been destroyed/);
    });

    it('returns a string response from the handler', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      expect(
        await w.webContents.executeJavaScript(
          `LanguageModel.create().then(async (model) => { const collect = ${collectStream}; return collect(model.promptStreaming("Hi")); })`
        )
      ).to.equal('foobar');
    });

    it('returns a ReadableStream response from the handler', async () => {
      await forkAndRegisterHandler('streaming-language-model.js');

      expect(
        await w.webContents.executeJavaScript(
          `LanguageModel.create().then(async (model) => { const collect = ${collectStream}; return collect(model.promptStreaming("Hi")); })`
        )
      ).to.equal('Hello World');
    });

    it('passes string input to the handler', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');

      const message = await listenForMessage(aiHandler, 'prompt-called', async () => {
        await w.webContents.executeJavaScript(
          `LanguageModel.create().then(async (model) => { const collect = ${collectStream}; return collect(model.promptStreaming('hello world')); })`
        );
      });

      expect(message.input).to.deep.equal([
        { role: 'user', content: [{ type: 'text', value: 'hello world' }], prefix: false }
      ]);
    });

    it('passes LanguageModelMessage[] input to the handler', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');

      const input = [
        { role: 'user', content: [{ type: 'text', value: 'hello' }] },
        { role: 'assistant', content: [{ type: 'text', value: 'hi' }] }
      ];

      const message = await listenForMessage(aiHandler, 'prompt-called', async () => {
        await w.webContents.executeJavaScript(
          `LanguageModel.create().then(async (model) => { const collect = ${collectStream}; return collect(model.promptStreaming(${JSON.stringify(input)})); })`
        );
      });

      expect(message.input).to.deep.equal(input.map((msg) => ({ ...msg, prefix: false })));
    });

    it('passes responseConstraint option to the handler', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');

      const responseConstraint = { type: 'object', properties: { name: { type: 'string' } } };

      const message = await listenForMessage(aiHandler, 'prompt-called', async () => {
        await w.webContents.executeJavaScript(
          `LanguageModel.create().then(async (model) => { const collect = ${collectStream}; return collect(model.promptStreaming('test', { responseConstraint: ${JSON.stringify(responseConstraint)} })); })`
        );
      });

      expect(message.options.responseConstraint).to.deep.equal(responseConstraint);
    });

    it('plumbs the abort signal through', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-prompt-response', value: 'wait-for-abort' });

      const message = await listenForMessage(aiHandler, 'prompt-aborted', async () => {
        await expect(
          w.webContents.executeJavaScript(
            `LanguageModel.create().then(async (model) => { const collect = ${collectStream}; return collect(model.promptStreaming("test", { signal: AbortSignal.timeout(500) })); }).catch(err => { throw err.message; })`
          )
        ).to.eventually.be.rejectedWith(/signal timed out/);
      });

      expect(message).not.null();
    });

    it('updates contextUsage after a prompt', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      expect(
        await w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => ({ contextUsage: model.contextUsage }))'
        )
      ).to.deep.equal({ contextUsage: 0 });
      expect(
        await w.webContents.executeJavaScript(
          `LanguageModel.create().then(async (model) => { const collect = ${collectStream}; await collect(model.promptStreaming("hello world")); return { contextUsage: model.contextUsage }; })`
        )
      ).to.deep.equal({ contextUsage: 10 });
    });
  });

  describe('LanguageModel.append()', () => {
    it('rejects when handler promise rejects', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-append-response', value: 'reject' });

      await expect(
        w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.append("Test")).catch(err => { throw err.message; })'
        )
      ).to.eventually.be.rejectedWith(/error occurred/);
    });

    it('rejects after the model has been destroyed', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      await expect(
        w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => { model.destroy(); return model.append("Test") }).catch(err => { throw err.message; })'
        )
      ).to.eventually.be.rejectedWith(/has been destroyed/);
    });

    it('rejects if the utility process dies during append', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-append-response', value: 'wait-for-abort' });

      const { promise } = await waitForMessage(aiHandler, 'append-called', () => {
        return w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.append("Test")).catch(err => { throw err.message; })'
        );
      });

      aiHandler.kill();
      await once(aiHandler, 'exit');

      await expect(promise).to.eventually.be.rejectedWith(/has been destroyed/);
    });

    it('rejects if the handler gets unregistered during append', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-append-response', value: 'wait-for-abort' });

      const { promise } = await waitForMessage(aiHandler, 'append-called', () => {
        return w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.append("Test")).catch(err => { throw err.message; })'
        );
      });

      w.webContents.session.registerLocalAIHandler(null);

      await expect(promise).to.eventually.be.rejectedWith(/has been destroyed/);
    });

    it('appends a message without producing a response', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      expect(
        await w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.append("Test")).catch(err => { throw err.message; })'
        )
      ).to.be.undefined();
    });

    it('plumbs the abort signal through', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-append-response', value: 'wait-for-abort' });

      const message = await listenForMessage(aiHandler, 'append-aborted', async () => {
        await expect(
          w.webContents.executeJavaScript(
            'LanguageModel.create().then(model => model.append("test", { signal: AbortSignal.timeout(500) })).catch(err => { throw err.message; })'
          )
        ).to.eventually.be.rejectedWith(/signal timed out/);
      });

      expect(message).not.null();
    });

    it('updates contextUsage after append', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      expect(
        await w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => ({ contextUsage: model.contextUsage }))'
        )
      ).to.deep.equal({ contextUsage: 0 });
      expect(
        await w.webContents.executeJavaScript(
          'LanguageModel.create().then(async (model) => { await model.append("hello world"); return { contextUsage: model.contextUsage } })'
        )
      ).to.deep.equal({ contextUsage: 5 });
    });
  });

  describe('LanguageModel.measureContextUsage()', () => {
    it('rejects if invalid value returned', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-measure-response', value: 'invalid' });

      await expect(
        w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.measureContextUsage("Test")).catch(err => { throw err.message; })'
        )
      ).to.eventually.be.rejectedWith(/usage cannot be calculated/);
    });

    it('rejects when handler promise rejects', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-measure-response', value: 'reject' });

      await expect(
        w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.measureContextUsage("Test")).catch(err => { throw err.message; })'
        )
      ).to.eventually.be.rejectedWith(/usage cannot be calculated/);
    });

    it('rejects after the model has been destroyed', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      await expect(
        w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => { model.destroy(); return model.measureContextUsage("Test") }).catch(err => { throw err.message; })'
        )
      ).to.eventually.be.rejectedWith(/has been destroyed/);
    });

    it('rejects if the utility process dies during call', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-measure-response', value: 'wait-for-abort' });

      const { promise } = await waitForMessage(aiHandler, 'measure-called', () => {
        return w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.measureContextUsage("Test")).catch(err => { throw err?.message ?? "Unknown Error"; })'
        );
      });

      aiHandler.kill();
      await once(aiHandler, 'exit');

      await expect(promise).to.eventually.be.rejected();
    });

    it('rejects if the handler gets unregistered during call', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-measure-response', value: 'wait-for-abort' });

      const { promise } = await waitForMessage(aiHandler, 'measure-called', () => {
        return w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.measureContextUsage("Test")).catch(err => { throw err?.message ?? "Unknown Error"; })'
        );
      });

      w.webContents.session.registerLocalAIHandler(null);

      await expect(promise).to.eventually.be.rejected();
    });

    it('returns the token count for the given input', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      expect(
        await w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.measureContextUsage("hello world"))'
        )
      ).to.equal(42);
    });

    // TODO(dsanders11): Upstream Chromium issue prevents this test from passing as
    //                   there's no Mojo connection to disconnect trip abort signal
    it.skip('plumbs the abort signal through', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-measure-response', value: 'wait-for-abort' });

      const message = await listenForMessage(aiHandler, 'measure-aborted', async () => {
        await expect(
          w.webContents.executeJavaScript(
            'LanguageModel.create().then(model => model.measureContextUsage("test", { signal: AbortSignal.timeout(500) })).catch(err => { throw err.message; })'
          )
        ).to.eventually.be.rejectedWith(/signal timed out/);
      });

      expect(message).not.null();
    });
  });

  describe('LanguageModel.clone()', () => {
    it('rejects when clone() returns a non-LanguageModel value', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-clone-response', value: 'invalid' });

      await expect(
        w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.clone()).catch(err => { throw err.message; })'
        )
      ).to.eventually.be.rejectedWith(/cannot be cloned/);
    });

    it('rejects when clone() promise rejects', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-clone-response', value: 'reject' });

      await expect(
        w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.clone()).catch(err => { throw err.message; })'
        )
      ).to.eventually.be.rejectedWith(/cannot be cloned/);
    });

    it('rejects after the original model has been destroyed', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      await expect(
        w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => { model.destroy(); return model.clone(); }).catch(err => { throw err.message; })'
        )
      ).to.eventually.be.rejectedWith(/has been destroyed/);
    });

    it('rejects if the utility process dies during clone', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-clone-response', value: 'wait-for-abort' });

      const { promise } = await waitForMessage(aiHandler, 'clone-called', () => {
        return w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.clone()).catch(err => { throw err.message; })'
        );
      });

      aiHandler.kill();
      await once(aiHandler, 'exit');

      await expect(promise).to.eventually.be.rejectedWith(/cannot be cloned/);
    });

    it('rejects if the handler gets unregistered during clone', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-clone-response', value: 'wait-for-abort' });

      const { promise } = await waitForMessage(aiHandler, 'clone-called', () => {
        return w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.clone()).catch(err => { throw err.message; })'
        );
      });

      w.webContents.session.registerLocalAIHandler(null);

      await expect(promise).to.eventually.be.rejectedWith(/cannot be cloned/);
    });

    it('returns a new LanguageModel instance', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      expect(
        await w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.clone()).then(cloned => cloned instanceof LanguageModel)'
        )
      ).to.equal(true);
    });

    it('preserves context from the original model', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      expect(
        await w.webContents.executeJavaScript(`
        LanguageModel.create().then(async (model) => {
          await model.prompt("hello");
          const cloned = await model.clone();
          return { contextUsage: cloned.contextUsage, contextWindow: cloned.contextWindow };
        })
      `)
      ).to.deep.equal({ contextUsage: 10, contextWindow: 12345 });
    });

    it('plumbs the abort signal through', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-clone-response', value: 'wait-for-abort' });

      const message = await listenForMessage(aiHandler, 'clone-aborted', async () => {
        await expect(
          w.webContents.executeJavaScript(
            'LanguageModel.create().then(model => model.clone({ signal: AbortSignal.timeout(500) })).catch(err => { throw err.message; })'
          )
        ).to.eventually.be.rejectedWith(/signal timed out/);
      });

      expect(message).not.null();
    });
  });

  describe('LanguageModel.destroy()', () => {
    it('destroys the model', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');

      const message = await listenForMessage(aiHandler, 'destroy-called', async () => {
        await expect(
          w.webContents.executeJavaScript(
            'LanguageModel.create().then(model => { model.destroy(); return model.prompt("Test"); }).catch(err => { throw err.message; })'
          )
        ).to.eventually.be.rejectedWith(/has been destroyed/);
      });

      expect(message).not.null();
    });

    it('aborts any in-progress prompt calls', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-prompt-response', value: 'wait-for-abort' });

      await w.webContents.executeJavaScript('LanguageModel.create().then(model => { window._model = model; })');

      const { promise } = await waitForMessage(aiHandler, 'prompt-called', () => {
        return w.webContents.executeJavaScript('window._model.prompt("Test").catch(err => { throw err.message; })');
      });
      const message = await listenForMessage(aiHandler, 'prompt-aborted', async () => {
        await w.webContents.executeJavaScript('window._model.destroy()');
      });

      await expect(promise).to.eventually.be.rejectedWith(/has been destroyed/);
      expect(message).not.null();
    });

    it('aborts any in-progress append calls', async () => {
      const aiHandler = await forkAndRegisterHandler('controllable-language-model.js');
      await sendControllableMessage(aiHandler, { command: 'set-append-response', value: 'wait-for-abort' });

      await w.webContents.executeJavaScript('LanguageModel.create().then(model => { window._model = model; })');

      const { promise } = await waitForMessage(aiHandler, 'append-called', () => {
        return w.webContents.executeJavaScript('window._model.append("Test").catch(err => { throw err.message; })');
      });
      const message = await listenForMessage(aiHandler, 'append-aborted', async () => {
        await w.webContents.executeJavaScript('window._model.destroy()');
      });

      await w.webContents.executeJavaScript('window._model.destroy()');

      await expect(promise).to.eventually.be.rejectedWith(/has been destroyed/);
      expect(message).not.null();
    });

    it('can be called multiple times without error', async () => {
      await forkAndRegisterHandler('basic-language-model.js');

      expect(
        await w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => { model.destroy(); model.destroy(); model.destroy(); return true; })'
        )
      ).to.equal(true);
    });
  });

  describe('setPromptAPIHandler()', () => {
    it('rejects if handler returns a promise', async () => {
      await forkAndRegisterHandler('promise-handler-language-model.js');

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('unavailable');
    });

    it('rejects if handler returns a non-class value', async () => {
      await forkAndRegisterHandler('non-class-handler-language-model.js');

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('unavailable');
    });

    it('rejects if handler returns a class not extending LanguageModel', async () => {
      await forkAndRegisterHandler('non-language-model-handler.js');

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('unavailable');
    });

    it('receives webContentsId in the details object', async () => {
      const aiHandler = await forkAndRegisterHandler('handler-details-language-model.js');

      const message = await listenForMessage(aiHandler, 'handler-called', async () => {
        await w.webContents.executeJavaScript('LanguageModel.availability()');
      });

      expect(message.details).to.have.property('webContentsId', w.webContents.id);
    });

    it('receives securityOrigin in the details object', async () => {
      const aiHandler = await forkAndRegisterHandler('handler-details-language-model.js');

      const message = await listenForMessage(aiHandler, 'handler-called', async () => {
        await w.webContents.executeJavaScript('LanguageModel.availability()');
      });

      expect(message.details).to.have.property('securityOrigin');
      expect(message.details.securityOrigin).to.be.a('string').and.not.be.empty();
    });

    it('is called once per webContentsId and securityOrigin pair', async () => {
      const aiHandler = await forkAndRegisterHandler('handler-details-language-model.js');

      const message = await listenForMessage(aiHandler, 'handler-called', async () => {
        await w.webContents.executeJavaScript('LanguageModel.availability()');
      });

      expect(message.callCount).to.equal(1);

      // Calling availability again should not trigger the handler again
      await w.webContents.executeJavaScript('LanguageModel.availability()');

      // Create a second window with the same session - should trigger handler again (different webContentsId)
      const w2 = new BrowserWindow({
        show: false,
        webPreferences: {
          session: w.webContents.session,
          enableBlinkFeatures: 'AIPromptAPI'
        }
      });
      await w2.loadFile(path.join(fixtures, 'api', 'blank.html'));

      const message2 = await listenForMessage(aiHandler, 'handler-called', async () => {
        await w2.webContents.executeJavaScript('LanguageModel.availability()');
      });

      expect(message2.callCount).to.equal(2);
    });

    it('rejects new bindings after handler is changed to return null', async () => {
      const aiHandler = await forkAndRegisterHandler('handler-details-language-model.js');

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('available');

      // Change the handler to return null for all new bindings
      await sendControllableMessage(aiHandler, { command: 'set-null-handler' });
      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('available');

      // Load a new page to get a fresh Prompt API binding
      await w.loadFile(path.join(fixtures, 'api', 'blank.html'));
      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('unavailable');
    });
  });

  describe('LanguageModel base class', () => {
    it('provides default no-op implementations for all methods', async () => {
      await forkAndRegisterHandler('default-language-model.js');

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('available');
      expect(
        await w.webContents.executeJavaScript('LanguageModel.create().then(model => model.prompt("Hi"))')
      ).to.equal('');
      expect(
        await w.webContents.executeJavaScript('LanguageModel.create().then(model => model.append("Hi"))')
      ).to.be.undefined();
      expect(
        await w.webContents.executeJavaScript('LanguageModel.create().then(model => model.measureContextUsage("Hi"))')
      ).to.equal(0);
      expect(
        await w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => model.clone()).then(cloned => cloned instanceof LanguageModel)'
        )
      ).to.equal(true);
    });

    it('can use the base LanguageModel class directly without subclassing', async () => {
      await forkAndRegisterHandler('default-language-model.js');

      expect(
        await w.webContents.executeJavaScript('LanguageModel.create().then(model => model instanceof LanguageModel)')
      ).to.equal(true);
      expect(
        await w.webContents.executeJavaScript(
          'LanguageModel.create().then(model => ({ contextUsage: model.contextUsage, contextWindow: model.contextWindow }))'
        )
      ).to.deep.equal({ contextUsage: 0, contextWindow: 0 });
    });
  });

  describe('session isolation', () => {
    it('applies to all windows using the same session', async () => {
      await forkAndRegisterHandler('default-language-model.js');

      expect(await w.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('available');

      const w2 = new BrowserWindow({
        show: false,
        webPreferences: {
          enableBlinkFeatures: 'AIPromptAPI'
        }
      });
      await w2.loadFile(path.join(fixtures, 'api', 'blank.html'));

      expect(await w2.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('available');
    });

    it('different sessions can use different handler processes', async () => {
      const ses1 = session.fromPartition('ai-isolation-1');
      const ses2 = session.fromPartition('ai-isolation-2');

      const w1 = new BrowserWindow({
        show: false,
        webPreferences: {
          session: ses1,
          enableBlinkFeatures: 'AIPromptAPI'
        }
      });
      const w2 = new BrowserWindow({
        show: false,
        webPreferences: {
          session: ses2,
          enableBlinkFeatures: 'AIPromptAPI'
        }
      });

      await Promise.all([
        w1.loadFile(path.join(fixtures, 'api', 'blank.html')),
        w2.loadFile(path.join(fixtures, 'api', 'blank.html'))
      ]);

      const aiHandler1 = utilityProcess.fork(getFixturePath('basic-language-model.js'));
      await once(aiHandler1, 'spawn');
      ses1.registerLocalAIHandler(aiHandler1);

      const aiHandler2 = utilityProcess.fork(getFixturePath('default-language-model.js'));
      await once(aiHandler2, 'spawn');
      ses2.registerLocalAIHandler(aiHandler2);

      try {
        // basic-language-model returns 'foobar'
        expect(
          await w1.webContents.executeJavaScript('LanguageModel.create().then(model => model.prompt("Hi"))')
        ).to.equal('foobar');
        // default-language-model returns ''
        expect(
          await w2.webContents.executeJavaScript('LanguageModel.create().then(model => model.prompt("Hi"))')
        ).to.equal('');
      } finally {
        ses1.registerLocalAIHandler(null);
        ses2.registerLocalAIHandler(null);
      }
    });

    it('clearing one session handler does not affect another', async () => {
      const ses1 = session.fromPartition('ai-isolation-clear-1');
      const ses2 = session.fromPartition('ai-isolation-clear-2');

      const w1 = new BrowserWindow({
        show: false,
        webPreferences: {
          session: ses1,
          enableBlinkFeatures: 'AIPromptAPI'
        }
      });
      const w2 = new BrowserWindow({
        show: false,
        webPreferences: {
          session: ses2,
          enableBlinkFeatures: 'AIPromptAPI'
        }
      });

      await Promise.all([
        w1.loadFile(path.join(fixtures, 'api', 'blank.html')),
        w2.loadFile(path.join(fixtures, 'api', 'blank.html'))
      ]);

      const aiHandler1 = utilityProcess.fork(getFixturePath('basic-language-model.js'));
      await once(aiHandler1, 'spawn');
      ses1.registerLocalAIHandler(aiHandler1);

      const aiHandler2 = utilityProcess.fork(getFixturePath('basic-language-model.js'));
      await once(aiHandler2, 'spawn');
      ses2.registerLocalAIHandler(aiHandler2);

      try {
        // Both should be available
        expect(await w1.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('available');
        expect(await w2.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('available');

        // Clear handler for session 1
        ses1.registerLocalAIHandler(null);

        // Session 1 should be unavailable, session 2 should still be available
        expect(await w1.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('unavailable');
        expect(await w2.webContents.executeJavaScript('LanguageModel.availability()')).to.equal('available');
      } finally {
        ses1.registerLocalAIHandler(null);
        ses2.registerLocalAIHandler(null);
      }
    });
  });
});
