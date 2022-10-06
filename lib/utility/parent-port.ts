import { EventEmitter } from 'events';
import { MessagePortMain } from '@electron/internal/browser/message-port-main';
const { createParentPort } = process._linkedBinding('electron_utility_parent_port');

export class ParentPort extends EventEmitter {
  #port: any
  constructor () {
    super();
    this.#port = createParentPort();
    this.#port.emit = (channel: string, event: {ports: any[]}) => {
      if (channel === 'message') {
        event = { ...event, ports: event.ports.map(p => new MessagePortMain(p)) };
      }
      this.emit(channel, event);
    };
  }

  start () {
    return this.#port.start();
  }

  pause () {
    return this.#port.pause();
  }

  postMessage (message: any) {
    return this.#port.postMessage(message);
  }
}
