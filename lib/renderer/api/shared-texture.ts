const binding = process._linkedBinding('electron_common_shared_texture');

// Textures sent with sharedTexture.sendSharedTexture() are imported and handed
// to the receiver natively (ElectronApiServiceImpl::ReceiveSharedTexture).
const sharedTexture = {
  subtle: binding,
  setSharedTextureReceiver: binding.setSharedTextureReceiver
};

export default sharedTexture;
