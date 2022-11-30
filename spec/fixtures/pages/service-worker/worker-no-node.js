self.clients.matchAll({ includeUncontrolled: true }).then((clients) => {
  if (!clients?.length) return;

  const msg = [typeof process, typeof setImmediate, typeof global, typeof Buffer].join(' ');
  clients[0].postMessage(msg);
});
