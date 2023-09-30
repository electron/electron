import { expect } from 'chai';
import * as http from 'node:http';
import { Socket } from 'node:net';
import { defer, listen } from './spec-helpers';

export async function getResponse (urlRequest: Electron.ClientRequest) {
  return new Promise<Electron.IncomingMessage>((resolve, reject) => {
    urlRequest.on('error', reject);
    urlRequest.on('abort', reject);
    urlRequest.on('response', (response) => resolve(response));
    urlRequest.end();
  });
}

export async function collectStreamBody (response: Electron.IncomingMessage | http.IncomingMessage) {
  return (await collectStreamBodyBuffer(response)).toString();
}

export function collectStreamBodyBuffer (response: Electron.IncomingMessage | http.IncomingMessage) {
  return new Promise<Buffer>((resolve, reject) => {
    response.on('error', reject);
    (response as NodeJS.EventEmitter).on('aborted', reject);
    const data: Buffer[] = [];
    response.on('data', (chunk) => data.push(chunk));
    response.on('end', (chunk?: Buffer) => {
      if (chunk) data.push(chunk);
      resolve(Buffer.concat(data));
    });
  });
}

export async function respondNTimes (fn: http.RequestListener, n: number): Promise<string> {
  const server = http.createServer((request, response) => {
    fn(request, response);
    // don't close if a redirect was returned
    if ((response.statusCode < 300 || response.statusCode >= 399) && n <= 0) {
      n--;
      server.close();
    }
  });
  const sockets: Socket[] = [];
  server.on('connection', s => sockets.push(s));
  defer(() => {
    server.close();
    for (const socket of sockets) {
      socket.destroy();
    }
  });
  return (await listen(server)).url;
}

export function respondOnce (fn: http.RequestListener) {
  return respondNTimes(fn, 1);
}

respondNTimes.routeFailure = false;

respondNTimes.toRoutes = (routes: Record<string, http.RequestListener>, n: number) => {
  return respondNTimes((request, response) => {
    if (Object.hasOwn(routes, request.url || '')) {
      (async () => {
        await Promise.resolve(routes[request.url || ''](request, response));
      })().catch((err) => {
        respondNTimes.routeFailure = true;
        console.error('Route handler failed, this is probably why your test failed', err);
        response.statusCode = 500;
        response.end();
      });
    } else {
      response.statusCode = 500;
      response.end();
      expect.fail(`Unexpected URL: ${request.url}`);
    }
  }, n);
};
respondOnce.toRoutes = (routes: Record<string, http.RequestListener>) => respondNTimes.toRoutes(routes, 1);

respondNTimes.toURL = (url: string, fn: http.RequestListener, n: number) => {
  return respondNTimes.toRoutes({ [url]: fn }, n);
};
respondOnce.toURL = (url: string, fn: http.RequestListener) => respondNTimes.toURL(url, fn, 1);

respondNTimes.toSingleURL = (fn: http.RequestListener, n: number) => {
  const requestUrl = '/requestUrl';
  return respondNTimes.toURL(requestUrl, fn, n).then(url => `${url}${requestUrl}`);
};
respondOnce.toSingleURL = (fn: http.RequestListener) => respondNTimes.toSingleURL(fn, 1);
