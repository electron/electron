import LanguageModelUtility from '@electron/internal/utility/api/language-model-utility';
import { ParentPort } from '@electron/internal/utility/parent-port';

import { EventEmitter } from 'events';
import { pathToFileURL } from 'url';

const v8Util = process._linkedBinding('electron_common_v8_util');

const entryScript: string = v8Util.getHiddenValue(process, '_serviceStartupScript');
process.argv.splice(1, 0, entryScript);

// These are used by C++ to more easily identify these objects.
// `stream/web` is required on first use rather than at startup: it evaluates
// all of Node's WHATWG streams.
v8Util.setHiddenValue(
  global,
  'isReadableStream',
  (val: unknown) => val instanceof (require('stream/web') as typeof import('stream/web')).ReadableStream
);
v8Util.setHiddenValue(global, 'isLanguageModel', (val: unknown) => val instanceof LanguageModelUtility);
v8Util.setHiddenValue(
  global,
  'isLanguageModelClass',
  (val: any) => Object.is(val, LanguageModelUtility) || val?.prototype instanceof LanguageModelUtility || false
);

// Import common settings.
require('@electron/internal/common/init');

process._linkedBinding('electron_browser_event_emitter').setEventEmitterPrototype(EventEmitter.prototype);

const parentPort: ParentPort = new ParentPort();
Object.defineProperty(process, 'parentPort', {
  enumerable: true,
  writable: false,
  value: parentPort
});

// Based on third_party/electron_node/lib/internal/worker/io.js
parentPort.on('newListener', (name: string) => {
  if (name === 'message' && parentPort.listenerCount('message') === 0) {
    parentPort.start();
  }
});

parentPort.on('removeListener', (name: string) => {
  if (name === 'message' && parentPort.listenerCount('message') === 0) {
    parentPort.pause();
  }
});

// Finally load entry script.
const { runEntryPointWithESMLoader } = __non_webpack_require__(
  'internal/modules/run_main'
) as typeof import('@node/lib/internal/modules/run_main');
const mainEntry = pathToFileURL(entryScript);

runEntryPointWithESMLoader(async (cascadedLoader: any) => {
  try {
    await cascadedLoader.import(mainEntry.toString(), undefined, Object.create(null));
  } catch (err) {
    const { internalBinding } = __non_webpack_require__('internal/bootstrap/realm') as {
      internalBinding: (name: string) => any;
    };
    internalBinding('errors').triggerUncaughtException(err);
  }
});
