// A small pipe transport for talking to Electron over CDP.
export class PipeTransport {
  private _pipeWrite: NodeJS.WritableStream | null;
  private _pendingMessage = '';

  onmessage?: (message: string) => void;

  constructor (pipeWrite: NodeJS.WritableStream, pipeRead: NodeJS.ReadableStream) {
    this._pipeWrite = pipeWrite;
    pipeRead.on('data', buffer => this._dispatch(buffer));
  }

  send (message: Object) {
    this._pipeWrite!.write(JSON.stringify(message));
    this._pipeWrite!.write('\0');
  }

  _dispatch (buffer: Buffer) {
    let end = buffer.indexOf('\0');
    if (end === -1) {
      this._pendingMessage += buffer.toString();
      return;
    }
    const message = this._pendingMessage + buffer.toString(undefined, 0, end);
    if (this.onmessage) { this.onmessage.call(null, JSON.parse(message)); }

    let start = end + 1;
    end = buffer.indexOf('\0', start);
    while (end !== -1) {
      const message = buffer.toString(undefined, start, end);
      if (this.onmessage) { this.onmessage.call(null, JSON.parse(message)); }
      start = end + 1;
      end = buffer.indexOf('\0', start);
    }
    this._pendingMessage = buffer.toString(undefined, start);
  }
}
