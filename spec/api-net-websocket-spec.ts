import { net, session } from 'electron/main';

import { expect } from 'chai';
import * as WSS from 'ws';

import * as http from 'node:http';
import { AddressInfo } from 'node:net';

import { defer } from './lib/spec-helpers';

type StartedServer = {
  url: string;
  server: http.Server;
  wss: WSS.Server;
};

async function startWSServer(
  onConnection?: (ws: WSS, request: http.IncomingMessage) => void,
  options: WSS.ServerOptions = {}
): Promise<StartedServer> {
  const server = http.createServer();
  // permessage-deflate is not the subject under test, so keep it off for determinism.
  const wss = new WSS.Server({ server, perMessageDeflate: false, ...options });
  if (onConnection) wss.on('connection', onConnection);
  await new Promise<void>((resolve) => server.listen(0, '127.0.0.1', resolve));
  defer(() => {
    wss.close();
    server.close();
  });
  const { port } = server.address() as AddressInfo;
  return { url: `ws://127.0.0.1:${port}`, server, wss };
}

function event<T = Event>(target: EventTarget, name: string): Promise<T> {
  return new Promise((resolve) => {
    target.addEventListener(name, (e) => resolve(e as T), { once: true });
  });
}

describe('net.WebSocket', () => {
  describe('constructor', () => {
    it('throws on invalid URLs', () => {
      expect(() => new net.WebSocket('not a url')).to.throw(/SyntaxError|Invalid URL/);
    });

    it('throws on non-ws/wss/http/https schemes', () => {
      expect(() => new net.WebSocket('ftp://example.com')).to.throw(DOMException);
    });

    it('throws on URLs with a fragment', () => {
      expect(() => new net.WebSocket('ws://example.com/#frag')).to.throw(DOMException);
    });

    it('rewrites http(s) to ws(s)', async () => {
      const { url } = await startWSServer((ws) => ws.close(1000));
      const ws = new net.WebSocket(url.replace('ws://', 'http://'));
      defer(() => ws.close());
      expect(ws.url).to.match(/^ws:/);
      await event(ws, 'close');
    });

    it('throws on invalid subprotocol tokens', () => {
      expect(() => new net.WebSocket('ws://example.com', 'bad protocol')).to.throw(DOMException);
    });

    it('throws on duplicate subprotocols', () => {
      expect(() => new net.WebSocket('ws://example.com', ['a', 'a'])).to.throw(DOMException);
    });

    it('exposes readyState constants', () => {
      expect(net.WebSocket.CONNECTING).to.equal(0);
      expect(net.WebSocket.OPEN).to.equal(1);
      expect(net.WebSocket.CLOSING).to.equal(2);
      expect(net.WebSocket.CLOSED).to.equal(3);
    });
  });

  describe('connection lifecycle', () => {
    it('opens and closes a connection', async () => {
      const { url } = await startWSServer((ws) => {
        ws.on('message', () => ws.close(1000, 'done'));
      });
      const ws = new net.WebSocket(url);
      defer(() => ws.close());

      expect(ws.readyState).to.equal(net.WebSocket.CONNECTING);
      await event(ws, 'open');
      expect(ws.readyState).to.equal(net.WebSocket.OPEN);
      ws.send('go');
      const closeEvent = await event<CloseEvent>(ws, 'close');
      expect(ws.readyState).to.equal(net.WebSocket.CLOSED);
      expect(closeEvent.code).to.equal(1000);
      expect(closeEvent.reason).to.equal('done');
      expect(closeEvent.wasClean).to.be.true();
    });

    it('reports the negotiated protocol', async () => {
      const { url } = await startWSServer(undefined, {
        handleProtocols: () => 'chat'
      });
      const ws = new net.WebSocket(url, ['chat', 'superchat']);
      defer(() => ws.close());
      await event(ws, 'open');
      expect(ws.protocol).to.equal('chat');
      ws.close();
      await event(ws, 'close');
    });

    it('fires error then close when the connection fails', async () => {
      const ws = new net.WebSocket('ws://127.0.0.1:1');
      const order: string[] = [];
      ws.addEventListener('error', () => order.push('error'));
      ws.addEventListener('close', () => order.push('close'));
      const e = await event<CloseEvent>(ws, 'close');
      expect(order).to.deep.equal(['error', 'close']);
      expect(ws.readyState).to.equal(net.WebSocket.CLOSED);
      // Network-level failures surface the failure reason in CloseEvent.reason.
      expect(e.code).to.equal(1006);
      expect(e.reason).to.match(/net::ERR_\w+/);
    });

    it('readyState is CLOSED inside the error handler', async () => {
      const ws = new net.WebSocket('ws://127.0.0.1:1');
      let stateInErrorHandler = -1;
      ws.addEventListener('error', () => {
        stateInErrorHandler = ws.readyState;
      });
      await event(ws, 'close');
      expect(stateInErrorHandler).to.equal(net.WebSocket.CLOSED);
    });

    it('can close while connecting', async () => {
      const ws = new net.WebSocket('ws://127.0.0.1:1');
      ws.close();
      expect(ws.readyState).to.equal(net.WebSocket.CLOSING);
      const e = await event<CloseEvent>(ws, 'close');
      expect(e.code).to.equal(1006);
      expect(e.wasClean).to.be.false();
    });

    it('validates close() arguments', async () => {
      const { url } = await startWSServer();
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      await event(ws, 'open');
      expect(() => ws.close(1234)).to.throw(DOMException);
      expect(() => ws.close(1000, 'x'.repeat(124))).to.throw(DOMException);
      ws.close(3001, 'bye');
      const e = await event<CloseEvent>(ws, 'close');
      expect(e.code).to.equal(3001);
      expect(e.reason).to.equal('bye');
    });
  });

  describe('messaging', () => {
    it('echoes text messages', async () => {
      const { url } = await startWSServer((ws) => {
        ws.on('message', (data: Buffer, isBinary: boolean) => ws.send(data, { binary: isBinary }));
      });
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      await event(ws, 'open');
      ws.send('hello, 世界');
      const e = await event<MessageEvent>(ws, 'message');
      expect(e.data).to.be.a('string');
      expect(e.data).to.equal('hello, 世界');
      ws.close();
      await event(ws, 'close');
    });

    it('echoes binary messages as Buffer by default', async () => {
      const { url } = await startWSServer((ws) => {
        ws.on('message', (data: Buffer, isBinary: boolean) => ws.send(data, { binary: isBinary }));
      });
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      await event(ws, 'open');
      ws.send(Buffer.from([1, 2, 3, 4]));
      const e = await event<MessageEvent>(ws, 'message');
      expect(Buffer.isBuffer(e.data)).to.be.true();
      expect([...e.data]).to.deep.equal([1, 2, 3, 4]);
      ws.close();
      await event(ws, 'close');
    });

    it('respects binaryType = arraybuffer', async () => {
      const { url } = await startWSServer((ws) => ws.send(Buffer.from([5, 6, 7])));
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      ws.binaryType = 'arraybuffer';
      await event(ws, 'open');
      const e = await event<MessageEvent>(ws, 'message');
      expect(e.data).to.be.an.instanceOf(ArrayBuffer);
      expect([...new Uint8Array(e.data)]).to.deep.equal([5, 6, 7]);
      ws.close();
      await event(ws, 'close');
    });

    it('respects binaryType = blob', async () => {
      const { url } = await startWSServer((ws) => ws.send(Buffer.from([8, 9])));
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      ws.binaryType = 'blob';
      await event(ws, 'open');
      const e = await event<MessageEvent>(ws, 'message');
      expect(e.data).to.be.an.instanceOf(Blob);
      expect([...new Uint8Array(await e.data.arrayBuffer())]).to.deep.equal([8, 9]);
      ws.close();
      await event(ws, 'close');
    });

    it('can send large messages that exceed the data pipe size', async () => {
      const { url } = await startWSServer((ws) => {
        ws.on('message', (m: Buffer) =>
          ws.send(Buffer.from([m.length & 0xff, (m.length >> 8) & 0xff, (m.length >> 16) & 0xff]))
        );
      });
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      await event(ws, 'open');
      const big = Buffer.alloc(2 * 1024 * 1024, 0x41);
      ws.send(big);
      const e = await event<MessageEvent>(ws, 'message');
      const [a, b, c] = e.data;
      expect(a + (b << 8) + (c << 16)).to.equal(big.length);
      ws.close();
      await event(ws, 'close');
    });

    it('can receive large messages that exceed the data pipe size', async () => {
      const big = Buffer.alloc(2 * 1024 * 1024, 0x42);
      const { url } = await startWSServer((ws) => ws.send(big));
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      await event(ws, 'open');
      const e = await event<MessageEvent>(ws, 'message');
      expect((e.data as Buffer).length).to.equal(big.length);
      expect((e.data as Buffer).equals(big)).to.be.true();
      ws.close();
      await event(ws, 'close');
    });

    it('throws InvalidStateError when sending while CONNECTING', () => {
      const ws = new net.WebSocket('ws://127.0.0.1:1');
      defer(() => ws.close());
      expect(() => ws.send('x')).to.throw(DOMException);
    });

    it('reports bufferedAmount', async () => {
      const { url } = await startWSServer();
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      await event(ws, 'open');
      expect(ws.bufferedAmount).to.equal(0);
      ws.close();
      await event(ws, 'close');
    });

    it('keeps incrementing bufferedAmount for sends after close', async () => {
      const { url } = await startWSServer();
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      await event(ws, 'open');
      ws.close();
      await event(ws, 'close');
      expect(ws.readyState).to.equal(net.WebSocket.CLOSED);
      ws.send('discarded');
      expect(ws.bufferedAmount).to.equal(Buffer.byteLength('discarded'));
    });

    it('counts bytes queued behind an async Blob send in bufferedAmount', async () => {
      const { url } = await startWSServer();
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      await event(ws, 'open');
      // The Blob holds the send queue open asynchronously; the string send is
      // chained behind it but must still be reflected in bufferedAmount
      // synchronously per the WebSocket spec.
      ws.send(new Blob(['four']));
      ws.send('three');
      expect(ws.bufferedAmount).to.be.at.least(4 + 5);
      ws.close();
      await event(ws, 'close');
    });

    it('preserves send order across Blob and non-Blob sends', async () => {
      const received: string[] = [];
      const { url } = await startWSServer((ws) => {
        ws.on('message', (m: Buffer) => received.push(m.toString()));
      });
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      await event(ws, 'open');
      ws.send('a');
      ws.send(new Blob(['b']));
      ws.send('c');
      ws.send(new Blob(['d']));
      ws.send('e');
      // Wait for the server to receive all five messages.
      await new Promise<void>((resolve) => {
        const check = () => {
          if (received.length >= 5) resolve();
          else setTimeout(check, 10);
        };
        check();
      });
      expect(received).to.deep.equal(['a', 'b', 'c', 'd', 'e']);
      ws.close();
      await event(ws, 'close');
    });
  });

  describe('Electron options', () => {
    it('sends extra headers with the handshake', async () => {
      let headers: http.IncomingHttpHeaders = {};
      const { url } = await startWSServer((ws, request) => {
        headers = request.headers;
        ws.close(1000);
      });
      const ws = new net.WebSocket(url, { headers: { 'X-Custom': 'electron' } });
      defer(() => ws.close());
      await event(ws, 'close');
      expect(headers['x-custom']).to.equal('electron');
    });

    it('defaults the Origin header to the http(s) origin of the WebSocket URL', async () => {
      let headers: http.IncomingHttpHeaders = {};
      const { url } = await startWSServer((ws, request) => {
        headers = request.headers;
        ws.close(1000);
      });
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      await event(ws, 'close');
      expect(headers.origin).to.equal(url.replace('ws://', 'http://'));
    });

    it('sends an Origin header when requested', async () => {
      let headers: http.IncomingHttpHeaders = {};
      const { url } = await startWSServer((ws, request) => {
        headers = request.headers;
        ws.close(1000);
      });
      const ws = new net.WebSocket(url, { origin: 'https://my-app.example' });
      defer(() => ws.close());
      await event(ws, 'close');
      expect(headers.origin).to.equal('https://my-app.example');
    });

    it('sends session cookies when useSessionCookies is true', async () => {
      const ses = session.fromPartition(`net-websocket-cookies-${Date.now()}`);
      let headers: http.IncomingHttpHeaders = {};
      const { url } = await startWSServer((ws, request) => {
        headers = request.headers;
        ws.close(1000);
      });
      await ses.cookies.set({ url: url.replace('ws://', 'http://'), name: 'ws', value: 'cookie' });
      const ws = new net.WebSocket(url, { session: ses, useSessionCookies: true });
      defer(() => ws.close());
      await event(ws, 'close');
      expect(headers.cookie).to.include('ws=cookie');
    });

    it('does not send session cookies by default', async () => {
      const ses = session.fromPartition(`net-websocket-nocookies-${Date.now()}`);
      let headers: http.IncomingHttpHeaders = {};
      const { url } = await startWSServer((ws, request) => {
        headers = request.headers;
        ws.close(1000);
      });
      await ses.cookies.set({ url: url.replace('ws://', 'http://'), name: 'ws', value: 'cookie' });
      const ws = new net.WebSocket(url, { session: ses });
      defer(() => ws.close());
      await event(ws, 'close');
      expect(headers.cookie).to.be.undefined();
    });
  });

  describe('event handler attributes', () => {
    it('supports onopen / onmessage / onclose', async () => {
      const { url } = await startWSServer((ws) => {
        ws.on('message', (m: Buffer) => ws.send(m.toString()));
      });
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      const events: string[] = [];
      ws.onopen = () => {
        events.push('open');
        ws.send('ping');
      };
      ws.onmessage = (e: MessageEvent) => {
        events.push(`message:${e.data}`);
        ws.close();
      };
      ws.onclose = () => events.push('close');
      await event(ws, 'close');
      expect(events).to.deep.equal(['open', 'message:ping', 'close']);
    });

    it('replaces a previously-assigned handler', async () => {
      const { url } = await startWSServer((ws) => ws.close(1000));
      const ws = new net.WebSocket(url);
      defer(() => ws.close());
      let firstCalled = false;
      ws.onopen = () => {
        firstCalled = true;
      };
      ws.onopen = () => {};
      await event(ws, 'close');
      expect(firstCalled).to.be.false();
    });
  });
});
