// Messages between the vitest CLI process (plain Node.js) and the Electron
// worker processes travel over a Node IPC channel. `serialization: 'advanced'`
// (what vitest's own forks pool uses) is not an option: it is V8's wire format,
// and Electron's V8 is newer than the host Node's, so the host cannot read what
// the worker writes. JSON is version independent, but vitest's task trees are
// cyclic (test -> suite -> file -> tasks) and carry a few non-JSON values, so
// both ends run messages through this encoder first. It flattens object
// identity (cycles and shared references) into an index table and tags the
// handful of types JSON would otherwise lose.

// First message a worker sends, before vitest's own protocol starts.
export const WORKER_READY_MESSAGE = 'electron-vitest-worker-ready';

const TYPE = '\u0000t';
const REF = '\u0000r';

type Encoded = { [TYPE]: string; v?: unknown } | { [REF]: number } | unknown;

export function serialize(value: unknown): string {
  const table: unknown[] = [];
  const seen = new Map<object, number>();

  const encode = (v: unknown): Encoded => {
    switch (typeof v) {
      case 'undefined':
        return { [TYPE]: 'u' };
      case 'bigint':
        return { [TYPE]: 'b', v: v.toString() };
      case 'number':
        if (Number.isNaN(v)) return { [TYPE]: 'n', v: 'NaN' };
        if (v === Infinity) return { [TYPE]: 'n', v: '+' };
        if (v === -Infinity) return { [TYPE]: 'n', v: '-' };
        return v;
      case 'function':
      case 'symbol':
        // Structured clone would throw here; JSON drops them. Dropping is the
        // more forgiving choice for stray values hanging off task objects.
        return { [TYPE]: 'u' };
      case 'object': {
        if (v === null) return null;
        const existing = seen.get(v);
        if (existing !== undefined) return { [REF]: existing };
        const id = table.length;
        seen.set(v, id);
        table.push(null);
        let out: unknown;
        if (Array.isArray(v)) {
          out = v.map(encode);
        } else if (v instanceof RegExp) {
          out = { [TYPE]: 'R', v: [v.source, v.flags] };
        } else if (v instanceof Date) {
          out = { [TYPE]: 'D', v: v.getTime() };
        } else if (v instanceof Map) {
          out = { [TYPE]: 'M', v: [...v.entries()].map(([k, e]) => [encode(k), encode(e)]) };
        } else if (v instanceof Set) {
          out = { [TYPE]: 'S', v: [...v].map(encode) };
        } else if (ArrayBuffer.isView(v)) {
          const bytes = new Uint8Array(v.buffer, v.byteOffset, v.byteLength);
          out = { [TYPE]: 'T', v: [v.constructor.name, Buffer.from(bytes).toString('base64')] };
        } else if (v instanceof ArrayBuffer) {
          out = { [TYPE]: 'A', v: Buffer.from(v).toString('base64') };
        } else if (v instanceof Error) {
          const o: Record<string, Encoded> = {
            name: encode(v.name),
            message: encode(v.message),
            stack: encode(v.stack)
          };
          for (const key of Object.getOwnPropertyNames(v)) {
            if (!(key in o)) o[key] = encode((v as any)[key]);
          }
          out = { [TYPE]: 'E', v: o };
        } else {
          const o: Record<string, Encoded> = {};
          for (const key of Object.keys(v)) {
            o[key] = encode((v as any)[key]);
          }
          out = o;
        }
        table[id] = out;
        return { [REF]: id };
      }
      default:
        return v;
    }
  };

  const root = encode(value);
  return JSON.stringify([root, table]);
}

export function deserialize(text: unknown): unknown {
  if (typeof text !== 'string') return text;
  const [root, table] = JSON.parse(text) as [Encoded, Encoded[]];
  const revived: unknown[] = new Array(table.length);
  const done: boolean[] = new Array(table.length).fill(false);

  const decode = (v: Encoded): unknown => {
    if (v === null || typeof v !== 'object') return v;
    if (Array.isArray(v)) return v.map(decode);
    if (REF in v) return revive((v as any)[REF]);
    if (TYPE in v) {
      const t = (v as any)[TYPE];
      const p = (v as any).v;
      switch (t) {
        case 'u':
          return undefined;
        case 'b':
          return BigInt(p);
        case 'n':
          return p === 'NaN' ? NaN : p === '+' ? Infinity : -Infinity;
      }
    }
    const o: Record<string, unknown> = {};
    for (const key of Object.keys(v)) o[key] = decode((v as any)[key]);
    return o;
  };

  const revive = (id: number): unknown => {
    if (done[id]) return revived[id];
    done[id] = true;
    const raw = table[id] as any;
    if (Array.isArray(raw)) {
      const arr: unknown[] = [];
      revived[id] = arr;
      for (const item of raw) arr.push(decode(item));
      return arr;
    }
    if (raw && typeof raw === 'object' && TYPE in raw) {
      const p = raw.v;
      switch (raw[TYPE]) {
        case 'R':
          return (revived[id] = new RegExp(p[0], p[1]));
        case 'D':
          return (revived[id] = new Date(p));
        case 'M': {
          const m = new Map();
          revived[id] = m;
          for (const [k, e] of p) m.set(decode(k), decode(e));
          return m;
        }
        case 'S': {
          const s = new Set();
          revived[id] = s;
          for (const e of p) s.add(decode(e));
          return s;
        }
        case 'T': {
          const buf = Buffer.from(p[1], 'base64');
          const Ctor = (globalThis as any)[p[0]] ?? Uint8Array;
          return (revived[id] =
            Ctor === Buffer
              ? buf
              : new Ctor(buf.buffer, buf.byteOffset, buf.byteLength / (Ctor.BYTES_PER_ELEMENT || 1)));
        }
        case 'A': {
          const buf = Buffer.from(p, 'base64');
          return (revived[id] = buf.buffer.slice(buf.byteOffset, buf.byteOffset + buf.byteLength));
        }
        case 'E': {
          const err: any = new Error();
          revived[id] = err;
          for (const key of Object.keys(p)) {
            Object.defineProperty(err, key, {
              value: decode(p[key]),
              enumerable: key !== 'stack' && key !== 'message',
              configurable: true,
              writable: true
            });
          }
          return err;
        }
      }
    }
    const o: Record<string, unknown> = {};
    revived[id] = o;
    for (const key of Object.keys(raw)) o[key] = decode(raw[key]);
    return o;
  };

  return decode(root);
}
