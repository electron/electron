const count = localStorage.getItem('count');

const { run, ipcRenderer } = window.api;

run().then(async () => {
  const count = await ipcRenderer.invoke('reload-successful');
  if (count < 3) location.reload();
}).catch(console.log);
