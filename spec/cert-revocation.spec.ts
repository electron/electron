import { afterAll, afterEach, beforeAll, describe, expect, it } from 'vitest';

import * as crypto from 'node:crypto';
import { once } from 'node:events';
import * as fs from 'node:fs';
import * as https from 'node:https';
import * as path from 'node:path';
import * as tls from 'node:tls';

import { ifdescribe, isTestingBindingAvailable, listen } from './lib/spec-helpers.ts';

const certPath = path.resolve(import.meta.dirname, 'fixtures', 'certificates');
const rootCA = fs.readFileSync(path.join(certPath, 'rootCA.pem'), 'utf8');
const intermediateCA = fs.readFileSync(path.join(certPath, 'intermediateCA.pem'), 'utf8');
const serverCert = fs.readFileSync(path.join(certPath, 'server.pem'), 'utf8');
const serverKey = fs.readFileSync(path.join(certPath, 'server.key'), 'utf8');

const spkiHash = (pem: string) =>
  crypto
    .createHash('sha256')
    .update(new crypto.X509Certificate(pem).publicKey.export({ type: 'spki', format: 'der' }))
    .digest();

ifdescribe(isTestingBindingAvailable())('certificate revocation', () => {
  const testing = process._linkedBinding('electron_common_testing');
  let server: https.Server;
  let port: number;

  beforeAll(async () => {
    server = https.createServer({ key: serverKey, cert: serverCert + intermediateCA }, (req, res) => {
      res.setHeader('Connection', 'close');
      res.end('ok');
    });
    port = (await listen(server)).port;
  });
  afterAll(() => new Promise<void>((resolve) => server.close(() => resolve())));

  describe('Node.js TLS', () => {
    afterEach(() => testing.clearNodeBlockedSPKIs());

    const connect = async (options: Partial<tls.ConnectionOptions> = {}) => {
      const socket = tls.connect({
        host: '127.0.0.1',
        port,
        servername: 'localhost',
        ca: [rootCA],
        rejectUnauthorized: false,
        ...options
      });
      try {
        await once(socket, 'secureConnect');
        return socket.authorizationError ? String(socket.authorizationError) : null;
      } finally {
        socket.destroy();
      }
    };

    it('reports a chain through a blocked intermediate key as revoked', async () => {
      await expect(connect(), 'before').resolves.not.toBe('CERT_REVOKED');
      testing.addNodeBlockedSPKI(spkiHash(intermediateCA).toString('hex'));
      await expect(connect(), 'after').resolves.toBe('CERT_REVOKED');
    });

    it('reports a blocked leaf key as revoked', async () => {
      testing.addNodeBlockedSPKI(spkiHash(serverCert).toString('hex'));
      await expect(connect()).resolves.toBe('CERT_REVOKED');
    });

    it('fails the handshake when rejectUnauthorized is set', async () => {
      testing.addNodeBlockedSPKI(spkiHash(intermediateCA).toString('hex'));
      const socket = tls.connect({
        host: '127.0.0.1',
        port,
        servername: 'localhost',
        ca: [rootCA],
        rejectUnauthorized: true
      });
      const [err] = await once(socket, 'error');
      socket.destroy();
      expect((err as NodeJS.ErrnoException).code).toBe('CERT_REVOKED');
    });

    it('applies to https.request', async () => {
      testing.addNodeBlockedSPKI(spkiHash(intermediateCA).toString('hex'));
      const req = https.get({ host: '127.0.0.1', port, servername: 'localhost', ca: [rootCA] });
      const [err] = await once(req, 'error');
      expect((err as NodeJS.ErrnoException).code).toBe('CERT_REVOKED');
    });

    it('does not affect chains without a blocked key', async () => {
      testing.addNodeBlockedSPKI(crypto.randomBytes(32).toString('hex'));
      await expect(connect()).resolves.not.toBe('CERT_REVOKED');
    });
  });
});
