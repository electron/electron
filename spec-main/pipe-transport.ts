// A small pipe transport for talking to Electron over CDP.
export class PipeTransport {
  #pipeWrite: NodeJS.WritableStream | null;
  #pendingMessage = '';

  onmessage?: (message: string) => void;

  constructor (pipeWrite: NodeJS.WritableStream, pipeRead: NodeJS.ReadableStream) {
    this.#pipeWrite = pipeWrite;
    pipeRead.on('data', buffer => this.#dispatch(buffer));
  }

  send (message: Object) {
    this.#pipeWrite!.write(JSON.stringify(message));
    this.#pipeWrite!.write('\0');
  }

  #dispatch (buffer: Buffer) {
    let end = buffer.indexOf('\0');
    if (end === -1) {
      this.#pendingMessage += buffer.toString();
      return;
    }
    const message = this.#pendingMessage + buffer.toString(undefined, 0, end);
    if (this.onmessage) { this.onmessage.call(null, JSON.parse(message)); }

    let start = end + 1;
    end = buffer.indexOf('\0', start);
    while (end !== -1) {
      const message = buffer.toString(undefined, start, end);
      if (this.onmessage) { this.onmessage.call(null, JSON.parse(message)); }
      start = end + 1;
      end = buffer.indexOf('\0', start);
    }
    this.#pendingMessage = buffer.toString(undefined, start);
  }
}
