const { fromPartition } = process._linkedBinding('electron_browser_session');

export default {
  fromPartition,
  get defaultSession () {
    return fromPartition('');
  }
};
