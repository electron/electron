import { allowAnyProtocol, toByteString } from '@electron/internal/common/api/net-client-request';

import { ClientRequestConstructorOptions, ClientRequest, IncomingMessage, Session as SessionT } from 'electron/main';

import { Readable, Writable, isReadable } from 'stream';

const { getFileInfoForFetch } = process._linkedBinding('electron_common_net');

type FetchInit = (RequestInit & { bypassCustomProtocolHandlers?: boolean }) | undefined;
type RequestFn = (options: ClientRequestConstructorOptions | string) => ClientRequest;

// A Response that net.fetch() answered from a file, for protocol.handle().
type FileResponse = {
  file: NodeJS.FileFetchBody;
  body: ReadableStream<Uint8Array>;
  signal: AbortSignal;
  release: () => void;
};
const fileResponses = new WeakMap<Response, FileResponse>();

function abortReason(signal: AbortSignal) {
  return signal.reason ?? new DOMException('The operation was aborted.', 'AbortError');
}

function createDeferredPromise<T, E extends Error = Error>(): {
  promise: Promise<T>;
  resolve: (x: T) => void;
  reject: (e: E) => void;
} {
  let res: (x: T) => void;
  let rej: (e: E) => void;
  const promise = new Promise<T>((resolve, reject) => {
    res = resolve;
    rej = reject;
  });

  return { promise, resolve: res!, reject: rej! };
}

// The Sec-Fetch-Mode fetchWithURLLoader sets from the request's mode. cors is
// the default mode, but we can't set mode=cors without an origin.
function modeHeader(req: Request) {
  return req.mode && (req.mode !== 'cors' || req.headers.get('origin')) ? req.mode : '';
}

// SimpleURLLoaderWrapper::Create() throws for these without navigate mode.
const navigationDestinations = new Set(['document', 'frame', 'iframe', 'fencedframe']);

// A GET or HEAD of a local file in the main process is answered from the file
// itself, without a request through the URL loader. A body, a Range header, a
// network share, an asar archive and an intercepted file: scheme go to the
// loader, as do the files that getFileInfoForFetch() leaves to it.
function canFetchFileDirectly(req: Request, init: FetchInit, session: SessionT | undefined): session is SessionT {
  return (
    session !== undefined &&
    req.url.startsWith('file:///') &&
    !/\.asar(?:\/|$)/i.test(req.url) &&
    (req.method === 'GET' || req.method === 'HEAD') &&
    req.body === null &&
    !req.headers.has('range') &&
    !(
      navigationDestinations.has(req.headers.get('sec-fetch-dest') ?? '') &&
      (req.headers.get('sec-fetch-mode') ?? modeHeader(req)) !== 'navigate'
    ) &&
    (!!init?.bypassCustomProtocolHandlers ||
      !(session.protocol.isProtocolHandled('file') || session.protocol.isProtocolRegistered('file')))
  );
}

// The URL loader options fetchWithURLLoader's ClientRequest would send for
// |req|: its headers, and what ClientRequest._startRequest() derives from
// them. webRequest listeners see the request they describe.
function urlLoaderOptions(req: Request) {
  const extraHeaders = new Map<string, [string, string]>();
  const mode = modeHeader(req);
  if (mode) extraHeaders.set('sec-fetch-mode', ['Sec-Fetch-Mode', mode]);
  for (const [name, value] of req.headers) extraHeaders.set(name, [name, value]);
  const header = (name: string) => extraHeaders.get(name)?.[1] ?? '';
  return {
    method: req.method,
    url: req.url,
    referrer: header('referer'),
    origin: header('origin'),
    hasUserActivation: header('sec-fetch-user') === '?1',
    mode: header('sec-fetch-mode'),
    destination: header('sec-fetch-dest'),
    extraHeaders: Object.fromEntries(extraHeaders.values())
  };
}

// The body of a file Response, read from |file| when first asked for, and a
// function that detaches it from |signal|.
function fileBody(file: NodeJS.FileFetchBody, signal: AbortSignal) {
  let onAbort: () => void;
  const release = () => signal.removeEventListener('abort', onAbort);
  const body = new ReadableStream<Uint8Array>(
    {
      start(controller) {
        onAbort = () => {
          file.discard();
          controller.error(abortReason(signal));
        };
        signal.addEventListener('abort', onAbort, { once: true });
      },
      pull(controller) {
        release();
        const bytes = file.read();
        if (bytes.byteLength > 0) controller.enqueue(bytes);
        controller.close();
      },
      cancel() {
        release();
        file.discard();
      }
    },
    // Nothing is read before it is asked for, so protocol.handle() can serve
    // a body that nothing has read itself.
    { highWaterMark: 0 }
  );
  return { body, release };
}

// Dispatches the webRequest events the URL loader's request for |req| would
// have. Resolves with the URL a listener redirected the request to, if any.
async function dispatchFileFetch(
  req: Request,
  webRequest: Electron.WebRequest,
  mimeType: string,
  lastModified: string
): Promise<string | undefined> {
  if (req.signal.aborted) throw abortReason(req.signal);
  const id = webRequest._startFileFetch(urlLoaderOptions(req));
  if (id === null) return undefined;

  const abort = () => webRequest._abortFileFetch(id);
  req.signal.addEventListener('abort', abort, { once: true });
  const verdict = await webRequest._fileFetchBeforeRequest(id).finally(() => {
    req.signal.removeEventListener('abort', abort);
  });
  if (req.signal.aborted) {
    webRequest._abortFileFetch(id);
    throw abortReason(req.signal);
  }
  if (verdict?.redirectURL) {
    // What ClientRequest does with the internal redirect a listener makes.
    if (req.redirect === 'error') throw new Error("Attempted to redirect, but redirect policy was 'error'");
    if (req.redirect === 'manual') throw new Error('Redirect was cancelled');
  }
  if (verdict?.error) throw new Error(verdict.error);
  if (verdict?.redirectURL) return verdict.redirectURL;
  webRequest._fileFetchResponded(id, mimeType, lastModified);
  return undefined;
}

// Runs getFileInfoForFetch() for |req|. The check can block on a slow or hung
// mount, so an abort rejects at once, as it does with the URL loader, and a
// body the check reads after that is dropped.
function checkFile(req: Request) {
  const check = getFileInfoForFetch(req.url, req.method === 'GET');
  const { signal } = req;
  return new Promise<Awaited<typeof check>>((resolve, reject) => {
    const onAbort = () => {
      reject(abortReason(signal));
      check.then(
        (late) => late?.body?.discard(),
        () => {}
      );
    };
    signal.addEventListener('abort', onAbort, { once: true });
    check.finally(() => signal.removeEventListener('abort', onAbort)).then(resolve, reject);
  });
}

// Answers a file: fetch as the URL loader would, with the same status,
// headers, body and webRequest events. Resolves with null when the loader has
// to.
async function fetchFile(
  req: Request,
  init: FetchInit,
  session: SessionT,
  request: RequestFn
): Promise<Response | null> {
  // For a GET, the check also reads the body, so later changes to the file
  // don't affect it.
  const info = await checkFile(req);
  if (!info) return null;

  let redirectURL: string | undefined;
  try {
    redirectURL = await dispatchFileFetch(req, session.webRequest, info.mimeType, info.lastModified);
    // A listener can abort the fetch, also from a microtask it queued.
    if (req.signal.aborted) throw abortReason(req.signal);
  } catch (err) {
    info.body?.discard();
    throw err;
  }
  if (redirectURL) {
    info.body?.discard();
    return fetchWithSession(
      new Request(redirectURL, req),
      { bypassCustomProtocolHandlers: init?.bypassCustomProtocolHandlers },
      session,
      request
    );
  }

  const resInit = {
    status: 200,
    statusText: 'OK',
    headers: { 'content-type': info.mimeType, 'last-modified': info.lastModified }
  };
  // Only a HEAD request has no body to read.
  if (!info.body) return new Response(null, resInit);
  const { body, release } = fileBody(info.body, req.signal);
  const res = new Response(body, resInit);
  fileResponses.set(res, { file: info.body, body, signal: req.signal, release });
  return res;
}

// Returns the body of a file Response from net.fetch() that nothing has read,
// for protocol.handle() to serve itself.
export function takeUntouchedFileResponse(res: Response): NodeJS.FileFetchBody | undefined {
  const entry = fileResponses.get(res);
  if (!entry || res.body !== entry.body || res.bodyUsed || entry.body.locked || entry.signal.aborted) return undefined;
  fileResponses.delete(res);
  entry.release();
  return entry.file;
}

export function fetchWithSession(
  input: RequestInfo,
  init: FetchInit,
  session: SessionT | undefined,
  request: RequestFn
): Promise<Response> {
  let req: Request;
  try {
    req = new Request(input, init);
  } catch (e: any) {
    return Promise.reject(e);
  }

  if (!req.signal.aborted && canFetchFileDirectly(req, init, session)) {
    return fetchFile(req, init, session, request).then((res) => res ?? fetchWithURLLoader(req, init, session, request));
  }
  return fetchWithURLLoader(req, init, session, request);
}

function fetchWithURLLoader(req: Request, init: FetchInit, session: SessionT | undefined, request: RequestFn) {
  const p = createDeferredPromise<Response>();
  if (req.signal.aborted) {
    // 1. Abort the fetch() call with p, request, null, and
    //    requestObject’s signal’s abort reason.
    const error = abortReason(req.signal);
    p.reject(error);

    if (req.body != null && isReadable(req.body as unknown as NodeJS.ReadableStream)) {
      req.body.cancel(error).catch((err) => {
        if (err.code === 'ERR_INVALID_STATE') {
          // Node bug?
          return;
        }
        throw err;
      });
    }

    // 2. Return p.
    return p.promise;
  }

  let locallyAborted = false;
  req.signal.addEventListener(
    'abort',
    () => {
      // 1. Set locallyAborted to true.
      locallyAborted = true;

      // 2. Abort the fetch() call with p, request, responseObject,
      //    and requestObject’s signal’s abort reason.
      const error = abortReason(req.signal);
      p.reject(error);
      if (req.body != null && isReadable(req.body as unknown as NodeJS.ReadableStream)) {
        req.body.cancel(error).catch((err) => {
          if (err.code === 'ERR_INVALID_STATE') {
            // Node bug?
            return;
          }
          throw err;
        });
      }

      r.abort();
    },
    { once: true }
  );

  const origin = req.headers.get('origin') ?? undefined;
  // We can't set credentials to same-origin unless there's an origin set.
  const credentials = req.credentials === 'same-origin' && !origin ? 'include' : req.credentials;

  const r = request(
    allowAnyProtocol({
      session,
      method: req.method,
      url: req.url,
      origin,
      credentials,
      cache: req.cache,
      referrerPolicy: req.referrerPolicy,
      redirect: req.redirect
    })
  );

  (r as any)._urlLoaderOptions.bypassCustomProtocolHandlers = !!init?.bypassCustomProtocolHandlers;

  const mode = modeHeader(req);
  if (mode) r.setHeader('Sec-Fetch-Mode', mode);

  for (const [k, v] of req.headers) {
    r.setHeader(k, v);
  }

  r.on('response', (resp: IncomingMessage) => {
    if (locallyAborted) return;
    try {
      const headers = new Headers();
      for (const [k, v] of Object.entries(resp.headers)) {
        if (Array.isArray(v)) {
          for (const item of v) {
            headers.append(k, toByteString(item));
          }
        } else if (typeof v === 'string') {
          headers.set(k, toByteString(v));
        }
      }
      const nullBodyStatus = [101, 204, 205, 304];
      const body =
        nullBodyStatus.includes(resp.statusCode) || req.method === 'HEAD'
          ? null
          : (Readable.toWeb(resp as unknown as Readable) as ReadableStream);
      const rResp = new Response(body, {
        headers,
        status: resp.statusCode,
        statusText: resp.statusMessage
      });
      (rResp as any).__original_resp = resp;
      // protocol.handle relays a Response that comes back untouched without
      // pumping its body through JS; it needs the loader and the exact stream.
      if (body) (rResp as any).__fetch = { request: r, body: rResp.body };
      p.resolve(rResp);
    } catch (err: any) {
      r.abort();
      p.reject(err);
    }
  });

  r.on('error', (err) => {
    p.reject(err);
  });

  // pipeTo expects a WritableStream<Uint8Array>. Node.js' Writable.toWeb returns WritableStream<any>,
  // which causes a TS structural mismatch.
  const writable = Writable.toWeb(r as unknown as Writable) as unknown as WritableStream<Uint8Array>;
  if (!req.body?.pipeTo(writable).then(() => r.end())) {
    r.end();
  }

  return p.promise;
}
