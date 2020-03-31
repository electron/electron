import { MessagePortMain } from '@electron/internal/browser/message-port-main';
const { createPair } = process.electronBinding('message_port');

export default class MessageChannelMain {
  port1: MessagePortMain;
  port2: MessagePortMain;
  constructor () {
    const { port1, port2 } = createPair();
    this.port1 = new MessagePortMain(port1);
    this.port2 = new MessagePortMain(port2);
  }
}
