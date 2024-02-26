import { EventEmitter } from 'events';

const { ipc } = process._linkedBinding('electron_renderer_ipc');

const internal = true;

type IPCHandler = (event: Electron.IpcRendererEvent, ...args: any[]) => any

class IpcRendererInternal extends EventEmitter {
  send (channel: string, ...args: any[]) {
    return ipc.send(internal, channel, args);
  }

  sendSync (channel: string, ...args: any[]) {
    return ipc.sendSync(internal, channel, args);
  }

  async invoke<T> (channel: string, ...args: any[]) {
    const { error, result } = await ipc.invoke<T>(internal, channel, args);
    if (error) {
      throw new Error(`Error invoking remote method '${channel}': ${error}`);
    }
    return result;
  };

  invokeSync<T> (command: string, ...args: any[]): T {
    const [error, result] = this.sendSync(command, ...args);

    if (error) {
      throw error;
    } else {
      return result;
    }
  }

  handle<T extends IPCHandler> (channel: string, handler: T) {
    this.on(channel, async (event, requestId, ...args) => {
      const replyChannel = `${channel}_RESPONSE_${requestId}`;
      try {
        event.sender.send(replyChannel, null, await handler(event, ...args));
      } catch (error) {
        event.sender.send(replyChannel, error);
      }
    });
  };
}

export const ipcRendererInternal = new IpcRendererInternal();
