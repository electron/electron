import { EventEmitter } from 'events';

const binding = process._linkedBinding('electron_utility_local_ai_handler');

Object.setPrototypeOf(binding, EventEmitter.prototype);

module.exports = binding;
