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
//                        loading/first_load and app_shell/first_load (one
//                        shell call, after the timing).
//   tracing              Chrome-style startup tracing (--trace-startup) for
//                        the `trace` and `app_shell_trace` suites, and
//                        trace_processor to run TRACE_QUERIES on each trace.
//                        A perfetto system trace (the `perfetto` probe)
//                        misses renderer events from before the renderer
//                        connects to traced. The traced
//                        suites run separately from the timed suites, so the
//                        tracing overhead stays out of their numbers.
// The app shell page has suites of its own (app_shell and app_shell_trace),
// so a version that can't run it never costs the other series a round:
// compare.mts drops every result of a crossbench run that failed.
export function probeConfigs(): Record<string, { probes: Record<string, unknown> }> {
  const configs = {
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
  return { ...configs, app_shell: configs.loading, app_shell_trace: configs.trace };
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

// Page configs for one target. `capabilities` comes from the harness app's --cb-capabilities output.
const hasPainted = "return performance.getEntriesByName('first-contentful-paint').length > 0";

// One crossbench page action, e.g. { action: 'get', url, ready_state }.
type Action = Record<string, string>;
type Pages = Record<string, Action[]>;

interface PageConfigOptions {
  capabilities?: Capabilities;
  baseUrl: string;
  fileRoot: string;
  // Where writeAppShellSite (lib/app-shell.mts) wrote the app shell page.
  appRoot: string;
  resizeCount?: number;
}

// The app shell page marks app-ready once its full first view is in the DOM,
// and keeps its latest LCP candidate as the app-lcp mark. The content's LCP
// always comes after app-ready (the window chrome before it is smaller), so
// waiting for both means the mark holds the final LCP, however slowly the
// version under test paints.
const appShellDone = `
if (location.protocol !== 'file:' || document.readyState !== 'complete') return false;
const ready = performance.getEntriesByName('app-ready')[0];
const lcp = performance.getEntriesByName('app-lcp')[0];
return !!ready && !!lcp && lcp.startTime > ready.startTime;`;

export function pageConfigs({ capabilities = {}, baseUrl, fileRoot, appRoot, resizeCount = 20 }: PageConfigOptions) {
  const firstLoad: Action[] = [
    { action: 'get', url: `${baseUrl}/pages/index.html`, ready_state: 'complete' },
    { action: 'wait_for_condition', timeout: '20s', condition: hasPainted },
    { action: 'wait', duration: '1s' }
  ];
  // Like a packaged app, the app shell is loaded from file://.
  const appShellLoad: Action[] = [
    { action: 'get', url: `file://${appRoot}/index.html`, ready_state: 'any' },
    { action: 'wait_for_condition', timeout: '30s', condition: appShellDone.trim() },
    { action: 'wait', duration: '500ms' }
  ];
  const loading: Pages = {
    // Memory is sampled once the page has painted and settled.
    first_load: [...firstLoad, { action: 'meminfo', title: 'loaded' }],
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
  const appShell: Pages = {
    first_load: [...appShellLoad, { action: 'meminfo', title: 'loaded' }],
    reload: [
      ...appShellLoad,
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
      { action: 'get', url: `${baseUrl}/pages/resize.html`, ready_state: 'complete' },
      { action: 'wait', duration: '500ms' },
      { action: 'js', script: `window.cbRunResize(${resizeCount});` },
      { action: 'wait_for_condition', timeout: '60s', condition: 'return window.__cbResizeDone === true' }
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

  return {
    loading: { pages: loading },
    trace: { pages: trace },
    file_loading: { pages: fileLoading },
    app_shell: { pages: appShell },
    app_shell_trace: { pages: { first_load: appShellLoad } }
  };
}
