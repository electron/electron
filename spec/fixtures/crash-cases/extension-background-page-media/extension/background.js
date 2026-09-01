navigator.mediaDevices.enumerateDevices().then(
  () => console.log('enumerated'),
  (err) => console.log('enumerate failed', err)
);
