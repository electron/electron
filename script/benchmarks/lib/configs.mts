// Generates the crossbench page and probe configs, and the N-file site used by
// the file-loading scenario. The configs are plain JSON (a subset of hjson).
import * as fs from 'node:fs';
import * as path from 'node:path';

import type { Capabilities } from './types.mts';

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

// Trace categories for the `trace` suite, kept to what the queries below need
// so the traced runs stay close to the untraced ones:
//   blink.user_timing  navigationStart and the page's cb-resize-* marks
//   loading            firstContentfulPaint, largestContentfulPaint::Candidate
//   benchmark          PipelineReporter (per-frame presentation timing)
//   ui                 Widget::OnNativeWidgetSizeChanged
export const TRACE_CATEGORIES = ['blink.user_timing', 'loading', 'benchmark', 'ui'];

// trace_processor queries crossbench runs on each traced repetition. Results
// land in <run>/trace_processor/<name>.json as a list of rows.
export const TRACE_QUERIES = {
  // Navigation start to FCP and to the last LCP candidate of the last
  // main-frame navigation that painted, leaving out crossbench's own data:
  // URL pages.
  page_load: `
WITH
  nav AS (
    SELECT EXTRACT_ARG(arg_set_id, 'debug.data.navigationId') AS navigation_id, MIN(ts) AS ts
    FROM slice
    WHERE name = 'navigationStart'
      AND EXTRACT_ARG(arg_set_id, 'debug.data.isOutermostMainFrame')
      AND EXTRACT_ARG(arg_set_id, 'debug.data.documentLoaderURL') NOT LIKE 'data:%'
    GROUP BY 1
  ),
  fcp AS (
    SELECT EXTRACT_ARG(arg_set_id, 'debug.data.navigationId') AS navigation_id, MIN(ts) AS ts
    FROM slice
    WHERE name = 'firstContentfulPaint'
    GROUP BY 1
  ),
  lcp AS (
    SELECT EXTRACT_ARG(arg_set_id, 'debug.data.navigationId') AS navigation_id, MAX(ts) AS ts
    FROM slice
    WHERE name = 'largestContentfulPaint::Candidate'
      AND EXTRACT_ARG(arg_set_id, 'debug.data.isOutermostMainFrame')
    GROUP BY 1
  )
SELECT (fcp.ts - nav.ts) / 1e6 AS fcp, (lcp.ts - nav.ts) / 1e6 AS lcp
FROM nav JOIN fcp USING (navigation_id) LEFT JOIN lcp USING (navigation_id)
ORDER BY nav.ts DESC
LIMIT 1`,
  // One row per BrowserWindow.setSize() between the page's cb-resize-start
  // and cb-resize-end marks: from the browser's native widget size change to
  // the presentation of the first renderer frame whose main frame committed
  // after the page saw the new size (its cb-resized mark, set by the resize
  // event). Frames are matched by their commit, not by the PipelineReporter's
  // start: that is the frame's BeginFrame timestamp, which can be earlier
  // than the resize even when the frame's main frame picks the resize up.
  // Comparing against it skipped the frame that showed the resize and, when
  // no later frame changed anything, matched the next resize's frame instead.
  resize: `
WITH
  bracket AS (
    SELECT
      (SELECT MIN(ts) FROM slice WHERE name = 'cb-resize-start') AS begin_ts,
      (SELECT MAX(ts) FROM slice WHERE name = 'cb-resize-end') AS end_ts
  ),
  resizes AS (
    SELECT s.ts, LEAD(s.ts) OVER (ORDER BY s.ts) AS next_ts
    FROM slice s
    JOIN thread_track tt ON s.track_id = tt.id
    JOIN thread USING (utid)
    JOIN process p USING (upid), bracket b
    WHERE s.name = 'Widget::OnNativeWidgetSizeChanged' AND p.name = 'Browser'
      AND s.ts BETWEEN b.begin_ts AND b.end_ts
  ),
  seen AS (
    SELECT r.ts, r.next_ts,
      (SELECT MIN(m.ts) FROM slice m
        WHERE m.name = 'cb-resized' AND m.ts >= r.ts AND (r.next_ts IS NULL OR m.ts < r.next_ts)) AS seen_ts
    FROM resizes r
  ),
  frames AS (
    SELECT c.ts + c.dur AS commit_ts, s.ts + s.dur AS presented_ts
    FROM slice s
    JOIN process_track pt ON s.track_id = pt.id
    JOIN process p USING (upid)
    JOIN slice c ON c.parent_id = s.id AND c.name = 'SendBeginMainFrameToCommit'
    WHERE s.name = 'PipelineReporter' AND p.name = 'Renderer'
      AND EXTRACT_ARG(s.arg_set_id, 'frame_reporter.state') IN ('STATE_PRESENTED_ALL', 'STATE_PRESENTED_PARTIAL')
  )
SELECT
  r.ts AS resize_ts,
  ((SELECT MIN(f.presented_ts) FROM frames f
    WHERE f.commit_ts >= r.seen_ts AND (r.next_ts IS NULL OR f.commit_ts < r.next_ts)) - r.ts) / 1e6 AS resize_ms
FROM seen r
ORDER BY r.ts`
};

// crossbench probe config per suite. Everything the suites report comes from
// crossbench's own probes:
//   performance.entries  paint (FCP), navigation timing and performance.mark()
//                        entries, read from the page after each story. The
//                        file-loading pages mark 'files-loaded' themselves,
//                        since only the page knows when all N files ran.
//   meminfo              per-process PSS/RSS from /proc/<pid>/smaps_rollup,
//                        taken by the `meminfo` action at the end of
//                        loading/first_load (one shell call, after the timing).
//   tracing              Chrome-style startup tracing (--trace-startup) for
//                        the `trace` suite, and trace_processor to run
//                        TRACE_QUERIES on each trace. A perfetto system trace
//                        (the `perfetto` probe) misses renderer events from
//                        before the renderer connects to traced. The trace
//                        suite runs separately from the timed suites, so the
//                        tracing overhead stays out of their numbers.
export function probeConfigs(): Record<string, { probes: Record<string, unknown> }> {
  return {
    loading: { probes: { 'performance.entries': {}, meminfo: {} } },
    file_loading: { probes: { 'performance.entries': {} } },
    trace: {
      probes: {
        tracing: { preset: 'empty', categories: TRACE_CATEGORIES },
        trace_processor: {
          queries: Object.entries(TRACE_QUERIES).map(([name, sql]) => ({ name, sql: sql.trim() }))
        }
      }
    }
  };
}

const pad = '// padding to make each file about 300 bytes\n'.repeat(6);

// Writes <root>/classic.html (N classic <script> tags) and <root>/module.html
// (one module importing N modules), VS Code dev-build style. Both mark
// 'files-loaded' once every file has been evaluated; the performance.entries
// probe reads the mark's time since navigation start.
export function writeFileLoadingSite(root: string, count: number) {
  fs.rmSync(root, { recursive: true, force: true });
  fs.mkdirSync(path.join(root, 'classic'), { recursive: true });
  fs.mkdirSync(path.join(root, 'module'), { recursive: true });
  const done = "performance.mark('files-loaded');";

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

// One crossbench page action, e.g. { action: 'get', url, ready_state }.
type Action = Record<string, string>;
type Pages = Record<string, Action[]>;

interface PageConfigOptions {
  // From the harness app's --cb-capabilities output.
  capabilities?: Capabilities;
  fileRoot: string;
  // Where writeAppShellSite (lib/app-shell.mts) wrote the app shell page.
  appRoot: string;
  resizeCount?: number;
}

// The app shell marks app-ready once its full first view is in the DOM, and
// keeps its latest LCP candidate as the app-lcp mark. The content's LCP always
// comes after app-ready (the window chrome before it is smaller), so waiting
// for both means the mark holds the final LCP, however slowly the version
// under test paints.
const appShellDone = `
if (location.protocol !== 'file:' || document.readyState !== 'complete') return false;
const ready = performance.getEntriesByName('app-ready')[0];
const lcp = performance.getEntriesByName('app-lcp')[0];
return !!ready && !!lcp && lcp.startTime > ready.startTime;`;

// Injected into the app shell by the resize story. It steps the window
// between two sizes with BrowserWindow.setSize() through the harness preload
// (window.cbHarness). The latency itself is measured from the trace (see
// TRACE_QUERIES): from the browser's Widget::OnNativeWidgetSizeChanged to the
// presentation of the first renderer frame that commits after the page sees
// the new size, which the cb-resized mark records. The cb-resize-start and
// cb-resize-end marks bracket the resizes so the window's initial sizing is
// left out. Each step waits for the resize event, two frames and a short idle
// period, so the next resize never overlaps the previous one's frame. The
// smaller size is below the app's 960px breakpoint, so every step also
// changes its layout.
function resizeScript(count: number) {
  return `(async () => {
  if (!window.cbHarness) {
    window.__cbResizeDone = true;
    return;
  }
  const nextFrame = () => new Promise((resolve) => requestAnimationFrame(() => resolve()));
  const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
  performance.mark('cb-resize-start');
  for (let i = 0; i < ${count}; i++) {
    const [width, height] = i % 2 ? [1280, 800] : [900, 600];
    const resized = new Promise((resolve) =>
      addEventListener('resize', () => {
        performance.mark('cb-resized');
        resolve();
      }, { once: true })
    );
    await window.cbHarness.setSize(width, height);
    await resized;
    await nextFrame();
    await nextFrame();
    await sleep(100);
  }
  performance.mark('cb-resize-end');
  window.__cbResizeDone = true;
})();`;
}

// Page configs for one target. The loading and trace suites load the app
// shell (lib/app-shell.mts) from file://, like a packaged app.
export function pageConfigs({ capabilities = {}, fileRoot, appRoot, resizeCount = 20 }: PageConfigOptions) {
  const firstLoad: Action[] = [
    { action: 'get', url: `file://${appRoot}/index.html`, ready_state: 'any' },
    { action: 'wait_for_condition', timeout: '30s', condition: appShellDone.trim() },
    { action: 'wait', duration: '500ms' }
  ];
  const loading: Pages = {
    // Memory is sampled once the page has painted and settled.
    first_load: [...firstLoad, { action: 'meminfo', title: 'loaded' }],
    reload: [
      ...firstLoad,
      { action: 'js', script: 'location.reload();' },
      {
        action: 'wait_for_condition',
        timeout: '30s',
        condition: `if (performance.getEntriesByType('navigation')[0].type !== 'reload') return false;${appShellDone}`
      },
      { action: 'wait', duration: '500ms' }
    ]
  };

  // Stories measured from a trace (TRACE_QUERIES), in their own invocation.
  const trace: Pages = { first_load: firstLoad };
  if (capabilities.setSize) {
    trace.resize = [
      ...firstLoad,
      { action: 'js', script: resizeScript(resizeCount) },
      { action: 'wait_for_condition', timeout: '90s', condition: 'return window.__cbResizeDone === true' }
    ];
  }

  const fileLoading: Pages = {};
  for (const strategy of FILE_STRATEGIES) {
    if (strategy.capability && !capabilities[strategy.capability]) continue;
    for (const fileKind of FILE_KINDS) {
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
          condition: `return location.protocol === '${strategy.scheme}:' && performance.getEntriesByName('files-loaded').length > 0`
        },
        { action: 'wait', duration: '500ms' }
      ];
    }
  }

  return { loading: { pages: loading }, trace: { pages: trace }, file_loading: { pages: fileLoading } };
}
