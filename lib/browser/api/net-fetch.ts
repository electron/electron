import { allowAnyProtocol } from '@electron/internal/common/api/net-client-request';

import { ClientRequestConstructorOptions, ClientRequest, IncomingMessage, Session as SessionT } from 'electron/main';

import { Writable, isReadable } from 'stream';

type TransferableFetchResponse = {
  loader: {
    canTransferResponse: () => boolean;
  };
  mimeType?: string;
};

export type FetchResponseInfo = TransferableFetchResponse & {
  canTransfer: boolean;
};

const transferableResponses = new WeakMap<ReadableStream, TransferableFetchResponse>();

export function getFetchResponseInfo(res: Response): FetchResponseInfo | undefined {
  if (!res.body) return;
  const response = transferableResponses.get(res.body);
  if (!response) return;
  return {
    ...response,
    canTransfer: !res.bodyUsed && !res.body.locked && response.loader.canTransferResponse()
  };
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

export function fetchWithSession(
  input: RequestInfo,
  init: (RequestInit & { bypassCustomProtocolHandlers?: boolean }) | undefined,
  session: SessionT | undefined,
  request: (options: ClientRequestConstructorOptions | string) => ClientRequest
) {
  const p = createDeferredPromise<Response>();
  let req: Request;
  try {
    req = new Request(input, init);
  } catch (e: any) {
    p.reject(e);
    return p.promise;
  }

  if (req.signal.aborted) {
    // 1. Abort the fetch() call with p, request, null, and
    //    requestObject’s signal’s abort reason.
    const error = (req.signal as any).reason ?? new DOMException('The operation was aborted.', 'AbortError');
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
      const error = (req.signal as any).reason ?? new DOMException('The operation was aborted.', 'AbortError');
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
  (r as any)._urlLoaderOptions.transferableResponse = true;

  // cors is the default mode, but we can't set mode=cors without an origin.
  if (req.mode && (req.mode !== 'cors' || origin)) {
    r.setHeader('Sec-Fetch-Mode', req.mode);
  }

  for (const [k, v] of req.headers) {
    r.setHeader(k, v);
  }

  r.on('response', (resp: IncomingMessage) => {
    if (locallyAborted) return;
    const headers = new Headers();
    for (const [k, v] of Object.entries(resp.headers)) {
      headers.set(k, Array.isArray(v) ? v.join(', ') : v);
    }
    const nullBodyStatus = [101, 204, 205, 304];
    const loader = (r as any)._urlLoader;
    const hasBody = !nullBodyStatus.includes(resp.statusCode) && req.method !== 'HEAD';
    const bodyReader = hasBody ? loader.createResponseBodyReader() : null;
    const body = hasBody
      ? new ReadableStream<Uint8Array>(
          {
            async pull(controller) {
              const buffer = new Uint8Array(64 * 1024);
              const bytesRead = await bodyReader.read(buffer);
              if (bytesRead === 0) {
                controller.close();
              } else {
                controller.enqueue(buffer.subarray(0, bytesRead));
              }
            },
            cancel() {
              r.abort();
            }
          },
          { highWaterMark: 0 }
        )
      : null;
    const rResp = new Response(body, {
      headers,
      status: resp.statusCode,
      statusText: resp.statusMessage
    });
    if (rResp.body) {
      transferableResponses.set(rResp.body, {
        loader,
        mimeType: (resp as any)._responseHead?.mimeType
      });
    }
    p.resolve(rResp);
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
