const { fromPartition } = process.electronBinding('session', 'browser');

export default {
  fromPartition,
  get defaultSession () {
    return fromPartition('');
  }
};
