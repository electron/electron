const { run, invoke } = window.api;

run().then(async () => {
  const count = await invoke('reload-successful');
  if (count < 3) location.reload();
}).catch(console.log);
