const binding = process._linkedBinding('electron_renderer_web_utils');

export const getPathForFile = binding.getPathForFile;
