import send from 'send';

import * as http from 'node:http';
import * as path from 'node:path';

/**
 * A tiny express-shaped router for specs that need to stand up a fake
 * server with a handful of routes (currently the autoUpdater specs). File
 * responses go through `send`, which is what express uses under the hood, so
 * HEAD requests, range requests, content types etc. behave the way the
 * updaters expect.
 */

export type RoutedRequest = http.IncomingMessage & {
  /** Returns the value of the named request header, like express' `req.header()`. */
  header(name: string): string | undefined;
};

export type RoutedResponse = http.ServerResponse & {
  /** Sets the status code and returns the response for chaining. */
  status(code: number): RoutedResponse;
  /** Ends the response, optionally with a plain text body. */
  send(body?: string): void;
  /** Ends the response with a JSON body. */
  json(body: unknown): void;
  /** Streams the given file as an attachment. */
  download(filePath: string): void;
};

export type RouteHandler = (req: RoutedRequest, res: RoutedResponse) => void;
export type Middleware = (req: RoutedRequest, res: RoutedResponse, next: () => void) => void;

export type RoutedServer = {
  /** Registers a middleware that runs before routing for every request. */
  use(middleware: Middleware): void;
  /**
   * Registers a handler for GET (and, as in express, HEAD) requests whose
   * path matches `pattern`. `:name` segments match any single path segment.
   */
  get(pattern: string, handler: RouteHandler): void;
  listen(port: number, host: string, callback: () => void): http.Server;
};

function compilePattern(pattern: string): RegExp {
  const source = pattern
    .split('/')
    .map((segment) => (segment.startsWith(':') ? '[^/]+' : segment.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')))
    .join('/');
  return new RegExp(`^${source}/?$`);
}

function decorateRequest(req: http.IncomingMessage): RoutedRequest {
  return Object.assign(req, {
    header(name: string) {
      const value = req.headers[name.toLowerCase()];
      return Array.isArray(value) ? value.join(', ') : value;
    }
  });
}

function decorateResponse(req: http.IncomingMessage, res: http.ServerResponse): RoutedResponse {
  const routed: RoutedResponse = Object.assign(res, {
    status(code: number) {
      res.statusCode = code;
      return routed;
    },
    send(body?: string) {
      if (body !== undefined) res.setHeader('Content-Type', 'text/plain; charset=utf-8');
      res.end(body);
    },
    json(body: unknown) {
      res.setHeader('Content-Type', 'application/json; charset=utf-8');
      res.end(JSON.stringify(body));
    },
    download(filePath: string) {
      res.setHeader('Content-Disposition', `attachment; filename="${path.basename(filePath)}"`);
      send(req, encodeURI(filePath))
        .on('error', (err: NodeJS.ErrnoException & { status?: number }) => {
          res.statusCode = err.status ?? 500;
          res.end();
        })
        .pipe(res);
    }
  });
  return routed;
}

export function createRoutedServer(): RoutedServer {
  const middlewares: Middleware[] = [];
  const routes: { matcher: RegExp; handler: RouteHandler }[] = [];

  const handleRequest = (rawReq: http.IncomingMessage, rawRes: http.ServerResponse) => {
    const req = decorateRequest(rawReq);
    const res = decorateResponse(rawReq, rawRes);
    const route = () => {
      const pathname = new URL(req.url!, 'http://localhost').pathname;
      const isGetLike = req.method === 'GET' || req.method === 'HEAD';
      const match = isGetLike ? routes.find(({ matcher }) => matcher.test(pathname)) : undefined;
      if (match) {
        match.handler(req, res);
      } else {
        res.status(404).send();
      }
    };
    const runMiddleware = (index: number) => {
      if (index < middlewares.length) {
        middlewares[index](req, res, () => runMiddleware(index + 1));
      } else {
        route();
      }
    };
    runMiddleware(0);
  };

  return {
    use: (middleware) => {
      middlewares.push(middleware);
    },
    get: (pattern, handler) => {
      routes.push({ matcher: compilePattern(pattern), handler });
    },
    listen: (port, host, callback) => http.createServer(handleRequest).listen(port, host, callback)
  };
}
