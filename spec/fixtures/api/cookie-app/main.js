const { app, session } = require('electron');

app.whenReady().then(async function () {
  const url = 'http://foo.bar';
  const persistentSession = session.fromPartition('persist:ence-test');
  const name = 'test';
  const value = 'true';

  const set = () => persistentSession.cookies.set({
    url,
    name,
    value,
    expirationDate: Date.now() + 60000,
    sameSite: 'strict'
  });

  const get = () => persistentSession.cookies.get({
    url
  });

  const maybeRemove = async (pred) => {
    if (pred()) {
      await persistentSession.cookies.remove(url, name);
    }
  };

  try {
    await maybeRemove(() => process.env.PHASE === 'one');
    const one = await get();
    await set();
    const two = await get();
    await maybeRemove(() => process.env.PHASE === 'two');
    const three = await get();

    process.stdout.write(`${one.length}${two.length}${three.length}`);
  } catch (e) {
    process.stdout.write(`ERROR : ${e.message}`);
  } finally {
    process.stdout.end();

    app.quit();
  }
});
