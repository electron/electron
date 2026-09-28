const { contextBridge } = process._linkedBinding('electron_renderer_context_bridge');

export default contextBridge;
