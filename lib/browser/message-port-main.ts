import { EventEmitter } from 'events';

export class MessagePortMain extends EventEmitter implements Electron.MessagePortMain {
  _internalPort: any;
  constructor (internalPort: any) {
    super();
    this._internalPort = internalPort;
    this._internalPort.emit = (channel: string, event: {ports: any[]}) => {
      if (channel === 'message') { event = { ...event, ports: event.ports.map(p => new MessagePortMain(p)) }; }
      this.emit(channel, event);
    };
  }

  start () {
    return this._internalPort.start();
  }

  close () {
    return this._internalPort.close();
  }

  postMessage (...args: any[]) {
    if (Array.isArray(args[1])) {
      args[1] = args[1].map((o: any) => o instanceof MessagePortMain ? o._internalPort : o);
    }
    return this._internalPort.postMessage(...args);
  }
}
