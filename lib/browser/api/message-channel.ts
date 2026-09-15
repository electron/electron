const { createPair } = process._linkedBinding('electron_browser_message_port');

export default class MessageChannelMain implements Electron.MessageChannelMain {
  port1: Electron.MessagePortMain;
  port2: Electron.MessagePortMain;
  constructor() {
    const { port1, port2 } = createPair();
    this.port1 = port1;
    this.port2 = port2;
  }
}
