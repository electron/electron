const binding = process._linkedBinding('electron_renderer_blink_utils');

export const getPathForFile = binding.getPathForFile;
