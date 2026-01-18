import { expect } from 'chai';
import { session, BrowserWindow } from 'electron';
import * as http from 'node:http';
import { listen } from './lib/spec-helpers';
import { once } from 'node:events';

describe('session.clearHostResolverCache()', () => {
    let server: http.Server;
    let serverUrl: string;
    let connections = 0;

    before(async () => {
        server = http.createServer((req, res) => {
            res.end('ok');
        });
        server.on('connection', () => {
            connections++;
        });
        const { port } = await listen(server);
        serverUrl = `http://127.0.0.1:${port}`;
    });

    after(() => {
        server.close();
    });

    it('closes active connections and forces new DNS resolution', async () => {
        const ses = session.fromPartition(`test-${Math.random()}`);
        const w = new BrowserWindow({ show: false, webPreferences: { session: ses } });

        // First load - establishes a connection
        await w.loadURL(serverUrl);
        const initialConnections = connections;
        expect(initialConnections).to.be.at.least(1);

        // Second load - should reuse the existing connection (keep-alive)
        await w.loadURL(serverUrl);
        expect(connections).to.equal(initialConnections, 'Should reuse connection');

        // Clear host resolver cache - should close all connections
        await ses.clearHostResolverCache();

        // Third load - must establish a NEW connection because the old one was closed
        await w.loadURL(serverUrl);
        expect(connections).to.be.greaterThan(initialConnections, 'Should have established a new connection after clearing cache');

        w.close();
    });
});
