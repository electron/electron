let ipc: NodeJS.IpcRendererImpl | undefined;

/**
 * Get IPCRenderer implementation for the current process.
 */
export function getIPCRenderer () {
  if (ipc) return ipc;
  const ipcBinding = process._linkedBinding('electron_renderer_ipc');
  switch (process.type) {
    case 'renderer':
      return (ipc = ipcBinding.createForRenderFrame());
    case 'service-worker':
      return (ipc = ipcBinding.createForServiceWorker());
    default:
      throw new Error(`Cannot create IPCRenderer for '${process.type}' process`);
  }
};
