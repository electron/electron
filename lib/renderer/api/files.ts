const binding = process._linkedBinding('electron_renderer_files');

export default {
  getPath (file: File | unknown/* FIXME: FileSystemHandle */) {
    return binding.getPath(file);
  }
};
