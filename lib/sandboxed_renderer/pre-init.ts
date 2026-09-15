// Pre-initialization code for sandboxed renderers.

import * as events from 'events';

declare const binding: {
  get: (name: string) => any;
  process: NodeJS.Process;
};

// Expose internal binding getter.
process._linkedBinding = binding.get;

const { EventEmitter } = events;

// Include properties from script 'binding' parameter.
Object.assign(process, binding.process);

// The process object created by webpack is not an event emitter, fix it so
// the API is more compatible with non-sandboxed renderers.
for (const prop of Object.keys(EventEmitter.prototype) as (keyof typeof process)[]) {
  if (Object.hasOwn(process, prop)) {
    delete process[prop];
  }
}
Object.setPrototypeOf(process, EventEmitter.prototype);
