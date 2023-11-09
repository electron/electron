import { EventEmitter } from 'events';
import { MessagePortMain } from '@electron/internal/browser/message-port-main';
const { createParentPort } = process._linkedBinding('electron_utility_parent_port');

export class ParentPort extends EventEmitter implements Electron.ParentPort {
  #port: ParentPort;
  constructor () {
    super();
    this.#port = createParentPort();
    this.#port.emit = (channel: string | symbol, event: { ports: any[] }) => {
      if (channel === 'message') {
        event = { ...event, ports: event.ports.map(p => new MessagePortMain(p)) };
      }
      this.emit(channel, event);
      return false;
    };
  }

  start () : void {
    this.#port.start();
  }

  pause () : void {
    this.#port.pause();
  }

  postMessage (message: any) : void {
    this.#port.postMessage(message);
  }
}
