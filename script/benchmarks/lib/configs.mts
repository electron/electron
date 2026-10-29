// Generates the crossbench page and probe configs, and the N-file site used by
// the file-loading scenario. The configs are plain JSON (a subset of hjson).
import * as fs from 'node:fs';
import * as path from 'node:path';

import type { Capabilities, TargetKind } from './types.mts';

// The file-loading strategies. Each custom scheme is registered by the
// harness app (see harness-app/main.js) and only exists when the Electron
// version under test has the API, so `capability` gates the story.
interface FileStrategy {
  name: string;
  scheme: string;
  capability?: string;
}

export const FILE_STRATEGIES: FileStrategy[] = [
  { name: 'file', scheme: 'file' },
  { name: 'register_file_protocol', scheme: 'cblegacy', capability: 'registerFileProtocol' },
  { name: 'handle_fs', scheme: 'cbfs', capability: 'handleFs' },
  { name: 'handle_net_fetch', scheme: 'cbnet', capability: 'handleNetFetch' }
];

export const FILE_KINDS = ['classic', 'module'];

// Runs after every loading story. WebDriver awaits the returned promise, which
// lets LCP (only exposed through PerformanceObserver) be read from the buffer.
const METRICS_JS = `
return new Promise((resolve) => {
  const metrics = Object.assign({}, window.__cbMetrics || {});
  const done = () => {
    const nav = performance.getEntriesByType('navigation')[0];
    if (nav) {
      metrics.dom_content_loaded = nav.domContentLoadedEventEnd;
      metrics.load_event_end = nav.loadEventEnd;
    }
    const fcp = performance.getEntriesByName('first-contentful-paint')[0];
    if (fcp) metrics.fcp = fcp.startTime;
    resolve(metrics);
  };
  const types = (window.PerformanceObserver && PerformanceObserver.supportedEntryTypes) || [];
  if (!types.includes('largest-contentful-paint')) return done();
  const timer = setTimeout(done, 1000);
  new PerformanceObserver((list) => {
    const entries = list.getEntries();
    if (entries.length) metrics.lcp = entries[entries.length - 1].startTime;
    clearTimeout(timer);
    done();
  }).observe({ type: 'largest-contentful-paint', buffered: true });
});`;

export function probeConfig() {
  return { probes: { js: { js: METRICS_JS } } };
}

const pad = '// padding to make each file about 300 bytes\n'.repeat(6);

// Writes <root>/classic.html (N classic <script> tags) and <root>/module.html
// (one module importing N modules), VS Code dev-build style. Both record the
// time from navigation start to "all files evaluated" as files_loaded_ms.
export function writeFileLoadingSite(root: string, count: number) {
  fs.rmSync(root, { recursive: true, force: true });
  fs.mkdirSync(path.join(root, 'classic'), { recursive: true });
  fs.mkdirSync(path.join(root, 'module'), { recursive: true });
  const done =
    "performance.mark('files-loaded'); window.__cbMetrics = { files_loaded_ms: performance.getEntriesByName('files-loaded')[0].startTime };";

  const tags: string[] = [];
  const imports: string[] = [];
  for (let i = 0; i < count; i++) {
    fs.writeFileSync(path.join(root, 'classic', `s${i}.js`), `window.__cbSum = (window.__cbSum || 0) + ${i};\n${pad}`);
    fs.writeFileSync(path.join(root, 'module', `m${i}.js`), `export const v${i} = ${i};\n${pad}`);
    tags.push(`<script src="classic/s${i}.js"></script>`);
    imports.push(`import { v${i} } from './module/m${i}.js';`);
  }
  const html = (title: string, body: string) =>
    `<!doctype html><html><head><meta charset="utf-8"><title>${title}</title>\n${body}\n</head><body><h1>${title}</h1></body></html>\n`;
  fs.writeFileSync(
    path.join(root, 'classic.html'),
    html(`${count} classic scripts`, `${tags.join('\n')}\n<script>${done}</script>`)
  );
  fs.writeFileSync(path.join(root, 'all.js'), `${imports.join('\n')}\nwindow.__cbSum = ${count};\n${done}\n`);
  fs.writeFileSync(
    path.join(root, 'module.html'),
    html(`${count} ES modules`, '<script type="module" src="all.js"></script>')
  );
}

// Page configs for one target. `kind` is 'electron' or 'chrome';
// `capabilities` comes from the harness app's --cb-capabilities output.
const hasPainted = "return performance.getEntriesByName('first-contentful-paint').length > 0";

// One crossbench page action, e.g. { action: 'get', url, ready_state }.
type Action = Record<string, string>;
type Pages = Record<string, Action[]>;

interface PageConfigOptions {
  kind: TargetKind;
  capabilities?: Capabilities;
  baseUrl: string;
  fileRoot: string;
  resizeCount?: number;
}

export function pageConfigs({ kind, capabilities = {}, baseUrl, fileRoot, resizeCount = 20 }: PageConfigOptions) {
  const loading: Pages = {
    first_load: [
      { action: 'get', url: `${baseUrl}/pages/index.html`, ready_state: 'complete' },
      { action: 'wait_for_condition', timeout: '20s', condition: hasPainted },
      { action: 'wait', duration: '1s' }
    ],
    reload: [
      { action: 'get', url: `${baseUrl}/pages/index.html`, ready_state: 'complete' },
      { action: 'wait', duration: '1s' },
      { action: 'js', script: 'location.reload();' },
      {
        action: 'wait_for_condition',
        timeout: '20s',
        condition:
          "return performance.getEntriesByType('navigation')[0].type === 'reload' && document.readyState === 'complete' && performance.getEntriesByName('first-contentful-paint').length > 0"
      },
      { action: 'wait', duration: '500ms' }
    ]
  };
  if (kind === 'electron' && capabilities.setSize) {
    loading.resize = [
      { action: 'get', url: `${baseUrl}/pages/resize.html`, ready_state: 'complete' },
      { action: 'wait', duration: '500ms' },
      { action: 'js', script: `window.cbRunResize(${resizeCount});` },
      { action: 'wait_for_condition', timeout: '60s', condition: 'return window.__cbResizeDone === true' }
    ];
  }

  const fileLoading: Pages = {};
  for (const strategy of FILE_STRATEGIES) {
    if (kind !== 'electron' && strategy.scheme !== 'file') continue;
    if (strategy.capability && !capabilities[strategy.capability]) continue;
    for (const fileKind of FILE_KINDS) {
      // Chrome blocks module scripts from file:// (opaque origin CORS).
      if (kind !== 'electron' && fileKind === 'module') continue;
      const page = `${fileRoot}/${fileKind}.html`;
      // crossbench's `get` only accepts well-known schemes, and file:// must
      // be a browser-initiated navigation, so custom schemes navigate from
      // about:blank with JS instead.
      const navigate: Action[] =
        strategy.scheme === 'file'
          ? [{ action: 'get', url: `file://${page}`, ready_state: 'any' }]
          : [
              { action: 'get', url: 'about:blank', ready_state: 'complete' },
              { action: 'js', script: `location.href = ${JSON.stringify(`${strategy.scheme}://local${page}`)};` }
            ];
      fileLoading[`${strategy.name}_${fileKind}`] = [
        ...navigate,
        {
          action: 'wait_for_condition',
          timeout: '60s',
          condition: `return location.protocol === '${strategy.scheme}:' && !!window.__cbMetrics && window.__cbMetrics.files_loaded_ms > 0`
        },
        { action: 'wait', duration: '500ms' }
      ];
    }
  }

  return { loading: { pages: loading }, file_loading: { pages: fileLoading } };
}
