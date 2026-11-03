const { app, session } = require('electron');

const binding = process._linkedBinding('electron_common_testing');

app.commandLine.appendSwitch('proxy-server', 'http://switch-proxy.invalid:1111');

const url = 'http://example.invalid/';

app
  .whenReady()
  .then(async () => {
    const own = session.fromPartition('own-proxy');
    const inherited = session.fromPartition('inherited-proxy');

    const resolveProxies = async () => {
      binding.flushSessionProxyConfigForTesting(own);
      binding.flushSessionProxyConfigForTesting(inherited);
      return {
        own: await own.resolveProxy(url),
        inherited: await inherited.resolveProxy(url)
      };
    };

    await own.setProxy({ proxyRules: 'http://session-proxy.invalid:2222' });
    const before = await resolveProxies();

    await binding.simulateNetworkServiceCrash();
    const after = await resolveProxies();

    process.stdout.write(JSON.stringify({ before, after }));
    app.quit();
  })
  .catch((error) => {
    console.error(error);
    app.exit(1);
  });
