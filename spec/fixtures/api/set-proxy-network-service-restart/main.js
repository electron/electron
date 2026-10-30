const { app, session } = require('electron');

const { setTimeout: delay } = require('node:timers/promises');

app.commandLine.appendSwitch('proxy-server', 'http://switch-proxy.invalid:1111');

const url = 'http://example.invalid/';

function killNetworkService() {
  const { pid } = app.getAppMetrics().find((m) => m.serviceName === 'network.mojom.NetworkService');
  const gone = new Promise((resolve) => {
    app.on('child-process-gone', function listener(event, details) {
      if (details.serviceName !== 'network.mojom.NetworkService') return;
      app.off('child-process-gone', listener);
      resolve();
    });
  });
  process.kill(pid);
  return gone;
}

app
  .whenReady()
  .then(async () => {
    const own = session.fromPartition('own-proxy');
    const inherited = session.fromPartition('inherited-proxy');
    await own.setProxy({ proxyRules: 'http://session-proxy.invalid:2222' });

    // setProxy() can resolve before the network service has taken the new config.
    for (let i = 0; i < 1000 && (await own.resolveProxy(url)) !== 'PROXY session-proxy.invalid:2222'; i++) {
      await delay(10);
    }
    const before = {
      own: await own.resolveProxy(url),
      inherited: await inherited.resolveProxy(url)
    };

    await killNetworkService();

    // resolveProxy() answers with an empty string while the service is down.
    for (let i = 0; i < 1000 && (await own.resolveProxy(url)) === ''; i++) {
      await delay(10);
    }

    // Sample for a while instead of once: if the switch were written again on
    // restart, it would reach the new NetworkContext shortly after the context
    // itself.
    const after = { own: new Set(), inherited: new Set() };
    for (let i = 0; i < 20; i++) {
      after.own.add(await own.resolveProxy(url));
      after.inherited.add(await inherited.resolveProxy(url));
      await delay(50);
    }
    after.own.delete('');
    after.inherited.delete('');

    process.stdout.write(
      JSON.stringify({
        before,
        after: { own: [...after.own], inherited: [...after.inherited] }
      })
    );
    app.quit();
  })
  .catch((error) => {
    console.error(error);
    app.exit(1);
  });
