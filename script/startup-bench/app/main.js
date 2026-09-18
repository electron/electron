// Minimal startup benchmark app: red data: URL, timestamps on every milestone.
const now = () => performance.timeOrigin + performance.now(); // epoch ms, sub-ms
const marks = { js_start: now() };
const mark = (name) => {
  if (!(name in marks)) marks[name] = now();
};
const emit = (obj) => process.stdout.write('BENCH:' + JSON.stringify(obj) + '\n');

const { app, BrowserWindow } = require('electron');
mark('after_require_electron');

const URL = process.env.BENCH_URL || 'data:text/html,<body style="margin:0;background:red">';
const SHOW_MODE = process.env.BENCH_SHOW_MODE || 'immediate'; // immediate | ready-to-show
const W = +(process.env.BENCH_W || 800),
  H = +(process.env.BENCH_H || 600);

if (process.env.BENCH_USER_DATA) app.setPath('userData', process.env.BENCH_USER_DATA);

app.on('will-finish-launching', () => mark('will_finish_launching'));
marks.process_creation_time = process.getCreationTime();

app.whenReady().then(() => {
  mark('ready');
  const win = new BrowserWindow({ x: 0, y: 0, width: W, height: H, show: SHOW_MODE === 'immediate' });
  mark('window_created');
  const wc = win.webContents;
  win.once('ready-to-show', () => {
    mark('ready_to_show');
    if (SHOW_MODE === 'ready-to-show') {
      win.show();
      mark('show_called');
    }
  });
  win.once('show', () => mark('win_show_event'));
  wc.once('did-start-loading', () => mark('did_start_loading'));
  wc.once('did-start-navigation', () => mark('did_start_navigation'));
  wc.once('render-process-gone', (e, d) => emit({ error: 'render-process-gone', d }));
  wc.once('dom-ready', () => mark('dom_ready'));
  wc.once('did-stop-loading', () => mark('did_stop_loading'));
  wc.once('did-finish-load', async () => {
    mark('did_finish_load');
    try {
      const r = await wc.executeJavaScript(`new Promise(res => {
        const grab = () => res(JSON.stringify({
          timeOrigin: performance.timeOrigin,
          paints: performance.getEntriesByType('paint').map(e => [e.name, e.startTime]),
          nav: (performance.getEntriesByType('navigation')[0] || {toJSON(){return null}}).toJSON(),
          raf2: performance.now()
        }));
        let n = 0;
        const tick = () => { if (performance.getEntriesByType('paint').length || ++n > 120) grab(); else requestAnimationFrame(tick); };
        requestAnimationFrame(tick);
      })`);
      marks.renderer = JSON.parse(r);
    } catch (e) {
      marks.renderer_error = String(e);
    }
    mark('renderer_probe_done');
    marks.metrics = app
      .getAppMetrics()
      .map((m) => ({ type: m.type, name: m.name, pid: m.pid, creationTime: m.creationTime, cpu: m.cpu }));
    marks.gpu = app.getGPUFeatureStatus();
    process.stdout.write('BENCH_DONE:' + JSON.stringify(marks) + '\n');
    if (process.env.BENCH_EXIT_WHEN_DONE) setTimeout(() => app.exit(0), +process.env.BENCH_EXIT_WHEN_DONE);
  });
  wc.loadURL(URL);
  mark('load_url_called');
});
process.on('SIGTERM', () => app.exit(0));
