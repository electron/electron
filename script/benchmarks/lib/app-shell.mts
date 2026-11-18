// Generates the "app shell" page for the loading and trace suites: a
// self-contained, deterministic stand-in for a typical Electron app's first
// window, loaded through file:// like a packaged app.
//
//   index.html             an empty #app root, three stylesheets, three
//                          deferred scripts and an inline LCP recorder
//   styles/*.css           about 1,100 rules: tokens, resets and utilities,
//                          the layout, and one block per component
//   scripts/vendor.js      a small module loader, virtual DOM (keyed diff),
//                          store and event delegation, plus generated helper
//                          and schema modules
//   scripts/locale.js      a message catalog with interpolation and plurals
//   scripts/app.js         generated feature modules (components, reducers,
//                          services) and the app's views and bootstrap
//   data/items.json        projects, people, labels and `rows` work items
//
// On load the app fetches data/items.json, evaluates its modules, renders the
// window chrome (title bar, sidebar with inline SVG icons, toolbar), waits
// for that to be painted (its FCP), then renders the summary (the largest
// element, and so the LCP), four feature panels and a row for every item
// into the same tree with the diff, and marks `app-ready`. The page marks:
//   app-shell-rendered  the chrome is in the DOM (not yet painted)
//   app-ready           the full first view is in the DOM
//   app-lcp             the latest largest-contentful-paint candidate's time,
//                       recorded by a PerformanceObserver because crossbench's
//                       performance.entries probe only reads paint, mark and
//                       navigation entries
//
// Everything comes from a seeded PRNG, so every run gets the same bytes. No
// network, fonts or third-party code are involved. The code must run on
// Electron 15 (Chromium 94), so it avoids newer syntax and APIs (no class
// static blocks, Array.prototype.at, structuredClone, CSS nesting, :has(),
// @layer or container queries).
import * as fs from 'node:fs';
import * as path from 'node:path';

export interface AppShellOptions {
  rows?: number;
  seed?: number;
}

// Mulberry32: small, fast and the same everywhere.
function createRandom(seed: number) {
  let state = seed >>> 0;
  const next = () => {
    state = (state + 0x6d2b79f5) >>> 0;
    let t = state;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
  const int = (min: number, max: number) => min + Math.floor(next() * (max - min + 1));
  function pick<T>(list: readonly T[]): T {
    return list[Math.floor(next() * list.length)];
  }
  return { next, int, pick, bool: (p = 0.5) => next() < p };
}
type Random = ReturnType<typeof createRandom>;

const NOUNS = (
  'account action activity adapter agent alert archive asset audit backup badge balance batch billing board ' +
  'bookmark branch bridge budget buffer build bundle cache calendar campaign canvas catalog channel chart ' +
  'checkout client cluster comment config contract cursor customer dashboard dataset deploy design device ' +
  'digest draft editor engine entry event export feature feedback filter folder form gateway graph guide ' +
  'handler history import inbox index invoice issue job journal kernel label layer layout ledger library ' +
  'license link listener locale manifest market media member message metric migration mirror model module ' +
  'monitor network note notice offer order outline package panel parser partner payment pipeline plan player ' +
  'policy portal preset preview profile project prompt queue quota record release report request review route ' +
  'runtime sample schedule schema screen script search section segment session setting shard sheet signal ' +
  'snapshot source space sprint stack storage stream summary survey table target task team template theme ' +
  'thread ticket timeline token topic tracker update upload usage vault vendor version widget window workflow'
).split(' ');
const VERBS = (
  'add adjust align archive audit build check clean close compare configure convert create debug define deploy ' +
  'document draft enable export fix handle improve index investigate load merge migrate monitor move optimize ' +
  'prepare publish refactor remove rename render report request resolve restore review rewrite run schedule ' +
  'ship simplify split sync test track update upgrade validate verify'
).split(' ');
const ADJECTIVES = (
  'active async basic cached central custom daily default dynamic early external final flaky global hidden ' +
  'initial internal large legacy local main manual minor native nested new nightly offline old partial pending ' +
  'primary private public quick remote shared silent slow stable static stale strict temporary unused visual weekly'
).split(' ');
const FIRST_NAMES = (
  'Ada Alan Amara Ana Arjun Bea Carlos Chen Dana Diego Elif Emma Farah Felix Grace Hana Ivan Jae Jonas Kai Lena ' +
  'Leo Maya Mei Nadia Noah Omar Priya Quinn Rafael Sara Sofia Tariq Uma Victor Wen Yara Yusuf Zoe'
).split(' ');
const LAST_NAMES = (
  'Abbott Bauer Castro Dubois Eriksen Fischer Garcia Haddad Ito Jensen Kowalski Larsen Moreau Nakamura Okafor ' +
  'Petrov Quispe Rossi Schmidt Tanaka Underwood Varga Weber Xu Yilmaz Zhang'
).split(' ');
const STATUSES = ['backlog', 'todo', 'in_progress', 'in_review', 'blocked', 'done', 'canceled'];
const PRIORITIES = ['none', 'low', 'medium', 'high', 'urgent'];
const COMPONENT_SUFFIXES = ['Panel', 'List', 'Card', 'View', 'Editor', 'Picker', 'Badge', 'Summary', 'Timeline'];
const ICON_NAMES = [
  'inbox',
  'user',
  'star',
  'clock',
  'folder',
  'tag',
  'search',
  'filter',
  'sort',
  'settings',
  'bell',
  'plus',
  'check',
  'alert',
  'archive',
  'chart',
  'calendar',
  'link',
  'lock',
  'flag',
  'bug',
  'doc',
  'code',
  'layers'
];

const capitalize = (word: string) => word[0].toUpperCase() + word.slice(1);
const camel = (words: string[]) => words[0] + words.slice(1).filter(Boolean).map(capitalize).join('');
const kebab = (name: string) => name.replace(/[A-Z]/g, (c) => `-${c.toLowerCase()}`);

// Unique camelCase names like "billingPanel" or "streamTimeline".
function featureNames(random: Random, count: number, suffixes: string[]): string[] {
  const names = new Set<string>();
  while (names.size < count) names.add(camel([random.pick(NOUNS), random.pick(suffixes)]));
  return [...names];
}

function sentence(random: Random, min: number, max: number) {
  const words: string[] = [];
  const length = random.int(min, max);
  for (let i = 0; i < length; i++) {
    words.push(i % 3 === 1 ? random.pick(ADJECTIVES) : i % 3 === 2 ? random.pick(VERBS) : random.pick(NOUNS));
  }
  return `${capitalize(words.join(' '))}.`;
}

// ---------------------------------------------------------------------------
// data/items.json

// A fixed "now" so relative dates render the same on every run.
const NOW = Date.UTC(2026, 9, 1, 12, 0, 0);
const DAY = 86_400_000;

function generateData(random: Random, rows: number) {
  const users = Array.from({ length: 40 }, (_, i) => {
    const first = FIRST_NAMES[i % FIRST_NAMES.length];
    const last = random.pick(LAST_NAMES);
    return { id: `u${i}`, name: `${first} ${last}`, email: `${first}.${last}@example.com`.toLowerCase() };
  });
  const projects = Array.from({ length: 16 }, (_, i) => ({
    id: `p${i}`,
    key: `${random.pick(NOUNS).slice(0, 3).toUpperCase()}${i}`,
    name: `${capitalize(random.pick(ADJECTIVES))} ${random.pick(NOUNS)}s`,
    icon: ICON_NAMES[i % ICON_NAMES.length],
    hue: (i * 47) % 360
  }));
  const labels = Array.from({ length: 24 }, (_, i) => ({
    id: `l${i}`,
    name: `${random.pick(ADJECTIVES)}-${random.pick(NOUNS)}`,
    hue: (i * 29 + 11) % 360
  }));
  const items = Array.from({ length: rows }, (_, i) => {
    const project = random.pick(projects);
    const updated = NOW - random.int(0, 300) * DAY - random.int(0, DAY);
    const labelIds = new Set<string>();
    for (let n = random.int(0, 3); n > 0; n--) labelIds.add(random.pick(labels).id);
    return {
      id: `${project.key}-${1000 + i}`,
      title: `${capitalize(random.pick(VERBS))} ${random.pick(ADJECTIVES)} ${random.pick(NOUNS)} ${random.pick(NOUNS)}`,
      project: project.id,
      status: random.pick(STATUSES),
      priority: random.pick(PRIORITIES),
      assignee: random.bool(0.85) ? random.pick(users).id : null,
      labels: [...labelIds],
      updated: new Date(updated).toISOString(),
      estimate: random.pick([1, 2, 3, 5, 8, 13]),
      progress: random.int(0, 20) * 5,
      comments: random.int(0, 40)
    };
  });
  return {
    version: 3,
    now: new Date(NOW).toISOString(),
    user: users[0].id,
    users,
    projects,
    labels,
    views: ['inbox', 'my_items', 'starred', 'recent', 'created', 'subscribed', 'drafts', 'archive'],
    items
  };
}

// ---------------------------------------------------------------------------
// Icons: 24x24 outline paths built from a few primitives.

function generateIcons(random: Random): Record<string, string[]> {
  const icons: Record<string, string[]> = {};
  for (const name of ICON_NAMES) {
    const paths: string[] = [];
    for (let n = random.int(2, 4); n > 0; n--) {
      const x = random.int(3, 12);
      const y = random.int(3, 12);
      const w = random.int(4, 21 - x);
      const hgt = random.int(4, 21 - y);
      switch (random.int(0, 3)) {
        case 0:
          paths.push(
            `M${x + 2} ${y}h${w - 4}a2 2 0 0 1 2 2v${hgt - 4}a2 2 0 0 1-2 2h-${w - 4}a2 2 0 0 1-2-2v-${hgt - 4}a2 2 0 0 1 2-2z`
          );
          break;
        case 1: {
          const r = Math.max(2, Math.floor(Math.min(w, hgt) / 2));
          paths.push(`M${x + r} ${y}a${r} ${r} 0 1 0 0.01 0z`);
          break;
        }
        case 2:
          paths.push(`M${x} ${y + hgt}l${Math.floor(w / 2)}-${hgt}l${Math.ceil(w / 2)} ${hgt}`);
          break;
        default:
          paths.push(`M${x} ${y}h${w}M${x} ${y + Math.floor(hgt / 2)}h${w}M${x} ${y + hgt}h${Math.floor(w / 2)}`);
      }
    }
    icons[name] = paths;
  }
  return icons;
}

// ---------------------------------------------------------------------------
// Stylesheets

function generateBaseCss(random: Random) {
  const rules: string[] = [];
  const palette: string[] = [];
  for (let i = 0; i < 12; i++) {
    const hue = (i * 30 + random.int(0, 20)) % 360;
    palette.push(`--color-${i}: hsl(${hue}, ${random.int(45, 75)}%, ${random.int(38, 55)}%);`);
    palette.push(`--color-${i}-soft: hsl(${hue}, ${random.int(55, 85)}%, ${random.int(90, 96)}%);`);
  }
  rules.push(`:root {
  --font-ui: system-ui, -apple-system, "Segoe UI", Roboto, Ubuntu, Cantarell, "Noto Sans", sans-serif;
  --font-mono: ui-monospace, "SF Mono", Menlo, Consolas, "Liberation Mono", monospace;
  --bg: #f7f8fa;
  --bg-elevated: #ffffff;
  --bg-sidebar: #eef0f4;
  --border: #dfe3ea;
  --text: #1d2330;
  --text-muted: #5d6677;
  --accent: #3b6fd8;
  --accent-soft: #e5edfb;
  --radius: 6px;
  --shadow: 0 1px 2px rgba(16, 24, 40, 0.06), 0 1px 3px rgba(16, 24, 40, 0.1);
  ${palette.join('\n  ')}
}`);
  rules.push(
    '*, *::before, *::after { box-sizing: border-box; }',
    'html, body { margin: 0; padding: 0; height: 100%; }',
    'body { font: 13px/1.45 var(--font-ui); color: var(--text); background: var(--bg); -webkit-font-smoothing: antialiased; }',
    'h1, h2, h3, h4, p, ul, ol, figure { margin: 0; padding: 0; }',
    'ul, ol { list-style: none; }',
    'button, input, select, textarea { font: inherit; color: inherit; }',
    'button { cursor: pointer; background: none; border: 0; padding: 0; }',
    'a { color: var(--accent); text-decoration: none; }',
    'a:hover { text-decoration: underline; }',
    'table { border-collapse: collapse; border-spacing: 0; }',
    'svg { flex: none; }',
    ':focus-visible { outline: 2px solid var(--accent); outline-offset: 1px; }',
    '::selection { background: var(--accent-soft); }',
    'code, kbd, pre { font-family: var(--font-mono); font-size: 12px; }'
  );
  // Utilities, as a CSS framework would ship them: spacing, sizing, flex,
  // text and color.
  const sides: Record<string, string[]> = {
    '': [''],
    t: ['-top'],
    r: ['-right'],
    b: ['-bottom'],
    l: ['-left'],
    x: ['-left', '-right'],
    y: ['-top', '-bottom']
  };
  for (const [prefix, property] of [
    ['m', 'margin'],
    ['p', 'padding']
  ]) {
    for (const [side, suffixes] of Object.entries(sides)) {
      for (let step = 0; step <= 12; step++) {
        const value = step === 0 ? '0' : `${step * 4}px`;
        rules.push(`.${prefix}${side}-${step} { ${suffixes.map((s) => `${property}${s}: ${value};`).join(' ')} }`);
      }
    }
  }
  for (let step = 1; step <= 24; step++) rules.push(`.w-${step * 8} { width: ${step * 8}px; }`);
  for (let step = 1; step <= 16; step++) rules.push(`.h-${step * 4} { height: ${step * 4}px; }`);
  for (let step = 0; step <= 8; step++) rules.push(`.gap-${step} { gap: ${step * 4}px; }`);
  for (const [name, value] of Object.entries({
    flex: 'display: flex;',
    'inline-flex': 'display: inline-flex;',
    grid: 'display: grid;',
    hidden: 'display: none;',
    'flex-col': 'flex-direction: column;',
    'flex-wrap': 'flex-wrap: wrap;',
    'flex-1': 'flex: 1 1 0%;',
    'flex-none': 'flex: none;',
    'items-center': 'align-items: center;',
    'items-start': 'align-items: flex-start;',
    'justify-between': 'justify-content: space-between;',
    'justify-end': 'justify-content: flex-end;',
    truncate: 'overflow: hidden; text-overflow: ellipsis; white-space: nowrap;',
    'text-right': 'text-align: right;',
    'text-center': 'text-align: center;',
    'font-medium': 'font-weight: 500;',
    'font-semibold': 'font-weight: 600;',
    'text-muted': 'color: var(--text-muted);',
    rounded: 'border-radius: var(--radius);',
    'rounded-full': 'border-radius: 9999px;',
    shadow: 'box-shadow: var(--shadow);',
    'sr-only': 'position: absolute; width: 1px; height: 1px; overflow: hidden; clip: rect(0 0 0 0);'
  })) {
    rules.push(`.${name} { ${value} }`);
  }
  for (const size of [10, 11, 12, 13, 14, 16, 18, 20, 24, 28]) rules.push(`.text-${size} { font-size: ${size}px; }`);
  for (let i = 0; i < 12; i++) {
    rules.push(`.fg-${i} { color: var(--color-${i}); }`);
    rules.push(`.bg-${i} { background-color: var(--color-${i}-soft); }`);
    rules.push(`.border-${i} { border-color: var(--color-${i}); }`);
    rules.push(`.hover\\:bg-${i}:hover { background-color: var(--color-${i}-soft); }`);
  }
  return rules;
}

function generateLayoutCss() {
  return [
    '#app { height: 100%; }',
    '.app { display: grid; grid-template-columns: 248px minmax(0, 1fr); grid-template-rows: 40px 44px minmax(0, 1fr) 24px; grid-template-areas: "titlebar titlebar" "sidebar toolbar" "sidebar main" "status status"; height: 100vh; }',
    '.titlebar { grid-area: titlebar; display: flex; align-items: center; gap: 12px; padding: 0 12px; background: #1f2633; color: #e8ecf3; -webkit-app-region: drag; }',
    '.titlebar__name { font-weight: 600; letter-spacing: 0.01em; }',
    '.titlebar__search { margin-left: auto; width: 320px; height: 26px; padding: 0 10px; border-radius: 5px; border: 1px solid #3a4456; background: #2a3243; color: inherit; -webkit-app-region: no-drag; }',
    '.titlebar__search::placeholder { color: #94a0b6; }',
    '.titlebar .icon { color: #9fb3d9; }',
    '.sidebar { grid-area: sidebar; overflow-y: auto; padding: 8px 8px 16px; background: var(--bg-sidebar); border-right: 1px solid var(--border); }',
    '.sidebar__section { margin-top: 14px; }',
    '.sidebar__heading { display: flex; align-items: center; justify-content: space-between; padding: 0 8px 4px; font-size: 11px; font-weight: 600; text-transform: uppercase; letter-spacing: 0.06em; color: var(--text-muted); }',
    '.nav-item { display: flex; align-items: center; gap: 8px; width: 100%; height: 28px; padding: 0 8px; border-radius: 5px; color: var(--text); text-align: left; }',
    '.nav-item:hover { background: rgba(31, 38, 51, 0.06); }',
    '.nav-item--active { background: var(--accent-soft); color: var(--accent); font-weight: 500; }',
    '.nav-item__label { flex: 1; min-width: 0; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }',
    '.nav-item__count { font-size: 11px; color: var(--text-muted); font-variant-numeric: tabular-nums; }',
    '.nav-item__dot { width: 8px; height: 8px; border-radius: 50%; }',
    '.label-cloud { display: flex; flex-wrap: wrap; gap: 4px; padding: 0 8px; }',
    '.toolbar { grid-area: toolbar; display: flex; align-items: center; gap: 8px; padding: 0 16px; border-bottom: 1px solid var(--border); background: var(--bg-elevated); }',
    '.toolbar__title { font-size: 14px; font-weight: 600; margin-right: 8px; }',
    '.toolbar__spacer { flex: 1; }',
    '.button { display: inline-flex; align-items: center; gap: 6px; height: 28px; padding: 0 10px; border: 1px solid var(--border); border-radius: 5px; background: var(--bg-elevated); }',
    '.button:hover { background: var(--bg); }',
    '.button--primary { background: var(--accent); border-color: var(--accent); color: #fff; }',
    '.button--primary:hover { background: #335fbd; }',
    '.chip { display: inline-flex; align-items: center; gap: 4px; height: 22px; padding: 0 8px; border-radius: 11px; border: 1px solid var(--border); font-size: 12px; background: var(--bg-elevated); }',
    '.chip--active { border-color: var(--accent); color: var(--accent); background: var(--accent-soft); }',
    '.main { grid-area: main; overflow: auto; background: var(--bg); }',
    '.main__loading { padding: 24px; color: var(--text-muted); }',
    '.summary { display: grid; grid-template-columns: minmax(0, 1fr) 300px; gap: 24px; padding: 24px 24px 16px; }',
    '.summary__title { font-size: 26px; line-height: 34px; font-weight: 600; letter-spacing: -0.01em; }',
    '.summary__text { margin-top: 8px; font-size: 17px; line-height: 27px; color: #384152; }',
    '.summary__stats { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 8px; align-content: start; }',
    '.stat { padding: 10px 12px; border: 1px solid var(--border); border-radius: var(--radius); background: var(--bg-elevated); }',
    '.stat__value { font-size: 18px; font-weight: 600; font-variant-numeric: tabular-nums; }',
    '.stat__label { font-size: 11px; color: var(--text-muted); }',
    '.insights { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 12px; padding: 0 24px 16px; }',
    '.table-wrap { margin: 0 24px 24px; border: 1px solid var(--border); border-radius: var(--radius); background: var(--bg-elevated); }',
    '.items { font-variant-numeric: tabular-nums; }',
    '.items__head, .row { display: flex; align-items: center; }',
    '.items__head { position: sticky; top: 0; z-index: 1; height: 32px; background: #f1f3f7; border-bottom: 1px solid var(--border); font-size: 11px; font-weight: 600; text-transform: uppercase; letter-spacing: 0.04em; color: var(--text-muted); }',
    '.row { height: 35px; border-bottom: 1px solid #eef0f4; }',
    '.row:hover { background: #f8f9fc; }',
    '.row.is-selected { background: var(--accent-soft); }',
    '.cell { flex: none; padding: 0 10px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }',
    '.col-icon { width: 36px; }',
    '.col-id { width: 92px; font-family: var(--font-mono); font-size: 12px; color: var(--text-muted); }',
    '.col-title { flex: 1 1 auto; min-width: 0; }',
    '.col-status { width: 120px; }',
    '.col-priority { width: 96px; }',
    '.col-assignee { width: 170px; }',
    '.col-updated { width: 116px; color: var(--text-muted); }',
    '.col-progress { width: 120px; }',
    '.item-title { font-weight: 500; }',
    '.item-labels { display: inline-flex; gap: 4px; margin-left: 8px; vertical-align: middle; }',
    '.label { display: inline-block; padding: 0 6px; border-radius: 9px; font-size: 11px; line-height: 18px; }',
    '.status { display: inline-flex; align-items: center; gap: 6px; }',
    '.status::before { content: ""; width: 8px; height: 8px; border-radius: 50%; background: currentColor; }',
    '.status--backlog { color: #8a93a5; }',
    '.status--todo { color: #5d6677; }',
    '.status--in_progress { color: #d18b16; }',
    '.status--in_review { color: #7a4fd1; }',
    '.status--blocked { color: #d13c3c; }',
    '.status--done { color: #2f9e5b; }',
    '.status--canceled { color: #a1a8b5; text-decoration: line-through; }',
    '.priority { font-size: 12px; }',
    '.priority--urgent { color: #d13c3c; font-weight: 600; }',
    '.priority--high { color: #d17316; }',
    '.priority--medium { color: #5d6677; }',
    '.priority--low, .priority--none { color: #a1a8b5; }',
    '.avatar { display: inline-flex; align-items: center; justify-content: center; width: 22px; height: 22px; margin-right: 6px; border-radius: 50%; font-size: 10px; font-weight: 600; color: #fff; vertical-align: middle; }',
    '.progress { height: 6px; border-radius: 3px; background: #e8ebf1; overflow: hidden; }',
    '.progress__bar { height: 100%; background: var(--accent); }',
    '.icon { width: 16px; height: 16px; fill: none; stroke: currentColor; stroke-width: 1.75; stroke-linecap: round; stroke-linejoin: round; vertical-align: middle; }',
    '.icon--sm { width: 14px; height: 14px; }',
    '.statusbar { grid-area: status; display: flex; align-items: center; gap: 16px; padding: 0 12px; font-size: 11px; color: var(--text-muted); background: var(--bg-elevated); border-top: 1px solid var(--border); }',
    '@media (max-width: 960px) { .app { grid-template-columns: 200px minmax(0, 1fr); } .summary { grid-template-columns: minmax(0, 1fr); } .insights { grid-template-columns: repeat(2, minmax(0, 1fr)); } .col-progress, .col-updated { display: none; } }',
    '@media (prefers-reduced-motion: reduce) { * { transition: none !important; animation: none !important; } }'
  ];
}

// One block of rules per component, BEM style. Most components never render
// at startup, so most of these rules match nothing, as in a real app.
function generateComponentCss(random: Random, components: string[]) {
  const rules: string[] = [];
  for (const name of components) {
    const c = `.c-${kebab(name)}`;
    const hue = random.int(0, 359);
    const pad = random.pick([8, 10, 12, 16]);
    rules.push(
      `${c} { display: flex; flex-direction: column; gap: ${random.pick([4, 6, 8])}px; padding: ${pad}px; border: 1px solid var(--border); border-radius: var(--radius); background: var(--bg-elevated); min-width: 0; }`,
      `${c}__header { display: flex; align-items: center; justify-content: space-between; gap: 8px; font-weight: 600; }`,
      `${c}__header h3 { font-size: ${random.pick([12, 13, 14])}px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }`,
      `${c}__body { display: flex; flex-direction: column; gap: 2px; font-size: 12px; }`,
      `${c}__body li { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; padding: 2px 0; }`,
      `${c}__footer { font-size: 11px; color: var(--text-muted); }`,
      `${c}__toggle { color: hsl(${hue}, 55%, 45%); font-size: 11px; }`,
      `${c}--active { border-color: hsl(${hue}, 60%, 70%); box-shadow: 0 0 0 1px hsl(${hue}, 60%, 85%); }`,
      `${c}--compact ${c}__body li { padding: 0; }`,
      `${c}--disabled { opacity: 0.6; pointer-events: none; }`,
      `${c}:hover ${c}__toggle { text-decoration: underline; }`
    );
  }
  return rules;
}

// ---------------------------------------------------------------------------
// scripts/vendor.js: the runtime every module uses.

const RUNTIME = String.raw`(function (global) {
  "use strict";
  var factories = Object.create(null);
  var cache = Object.create(null);
  function define(id, factory) {
    factories[id] = factory;
  }
  function require(id) {
    var cached = cache[id];
    if (cached) return cached.exports;
    var factory = factories[id];
    if (!factory) throw new Error("Unknown module " + id);
    var module = (cache[id] = { id: id, exports: {} });
    factory.call(module.exports, module, module.exports, require);
    return module.exports;
  }
  require.has = function (id) {
    return id in factories;
  };
  global.__define = define;
  global.__require = require;
})(window);

__define("core", function (module, exports, require) {
  "use strict";
  const SVG_NS = "http://www.w3.org/2000/svg";
  const EMPTY = Object.freeze({});
  const BOOLEAN_PROPS = new Set(["checked", "disabled", "hidden", "selected", "readonly", "multiple"]);

  // Virtual DOM. A vnode is { tag, props, children, key, el }; text vnodes
  // have tag "#text" and a text field.
  function h(tag, props) {
    const children = [];
    for (let i = 2; i < arguments.length; i++) flatten(arguments[i], children);
    return { tag, props: props || EMPTY, children, key: props && props.key != null ? props.key : undefined, el: null };
  }
  function text(value) {
    return { tag: "#text", props: EMPTY, children: [], text: String(value), key: undefined, el: null };
  }
  function flatten(child, out) {
    if (child == null || child === false || child === true) return;
    if (Array.isArray(child)) {
      for (let i = 0; i < child.length; i++) flatten(child[i], out);
    } else if (typeof child === "object") {
      out.push(child);
    } else {
      out.push(text(child));
    }
  }
  function styleText(style) {
    let out = "";
    for (const name in style) {
      if (style[name] == null) continue;
      out += name.replace(/[A-Z]/g, (c) => "-" + c.toLowerCase()) + ":" + style[name] + ";";
    }
    return out;
  }
  function setProp(el, name, value, svg) {
    if (name === "key" || name === "children") return;
    if (name === "class" || name === "className") {
      if (svg) el.setAttribute("class", value || "");
      else el.className = value || "";
    } else if (name === "style") {
      el.style.cssText = typeof value === "string" ? value : styleText(value || EMPTY);
    } else if (name === "value" && !svg) {
      if (el.value !== value) el.value = value == null ? "" : value;
    } else if (BOOLEAN_PROPS.has(name)) {
      el[name] = !!value;
    } else if (value == null || value === false) {
      el.removeAttribute(name);
    } else {
      el.setAttribute(name, value === true ? "" : value);
    }
  }
  function updateProps(el, oldProps, newProps, svg) {
    for (const name in oldProps) {
      if (!(name in newProps)) setProp(el, name, null, svg);
    }
    for (const name in newProps) {
      const value = newProps[name];
      if (oldProps[name] !== value || name === "value") setProp(el, name, value, svg);
    }
  }
  function createElement(vnode, svg) {
    if (vnode.tag === "#text") return (vnode.el = document.createTextNode(vnode.text));
    const isSvg = svg || vnode.tag === "svg";
    const el = isSvg ? document.createElementNS(SVG_NS, vnode.tag) : document.createElement(vnode.tag);
    for (const name in vnode.props) setProp(el, name, vnode.props[name], isSvg);
    const children = vnode.children;
    for (let i = 0; i < children.length; i++) el.appendChild(createElement(children[i], isSvg && vnode.tag !== "foreignObject"));
    return (vnode.el = el);
  }
  function patch(parent, oldNode, newNode, svg) {
    if (!oldNode) {
      parent.appendChild(createElement(newNode, svg));
      return newNode.el;
    }
    if (oldNode === newNode) return newNode.el;
    if (oldNode.tag !== newNode.tag || oldNode.key !== newNode.key) {
      parent.replaceChild(createElement(newNode, svg), oldNode.el);
      return newNode.el;
    }
    const el = (newNode.el = oldNode.el);
    if (newNode.tag === "#text") {
      if (oldNode.text !== newNode.text) el.nodeValue = newNode.text;
      return el;
    }
    const isSvg = svg || newNode.tag === "svg";
    updateProps(el, oldNode.props, newNode.props, isSvg);
    patchChildren(el, oldNode.children, newNode.children, isSvg);
    return el;
  }
  // Keyed children are matched by key, unkeyed ones by position.
  function patchChildren(el, oldChildren, newChildren, svg) {
    let keyed = null;
    for (let i = 0; i < oldChildren.length; i++) {
      const key = oldChildren[i].key;
      if (key !== undefined) (keyed || (keyed = new Map())).set(key, oldChildren[i]);
    }
    const used = new Set();
    for (let i = 0; i < newChildren.length; i++) {
      const next = newChildren[i];
      let prev;
      if (next.key !== undefined) prev = keyed && keyed.get(next.key);
      else if (oldChildren[i] && oldChildren[i].key === undefined) prev = oldChildren[i];
      if (prev && prev.tag === next.tag && !used.has(prev)) {
        used.add(prev);
        patch(el, prev, next, svg);
        const ref = el.childNodes[i];
        if (ref !== next.el) el.insertBefore(next.el, ref || null);
      } else {
        el.insertBefore(createElement(next, svg), el.childNodes[i] || null);
      }
    }
    for (let i = 0; i < oldChildren.length; i++) {
      const old = oldChildren[i];
      if (!used.has(old) && old.el && old.el.parentNode === el) el.removeChild(old.el);
    }
  }
  function render(container, vnode, previous) {
    if (previous) patch(container, previous, vnode, false);
    else patch(container, null, vnode, false);
    return vnode;
  }

  // Components with local state; setState re-renders on the next frame.
  const dirty = new Set();
  let frame = 0;
  function flush() {
    frame = 0;
    const list = Array.from(dirty);
    dirty.clear();
    for (const component of list) component.update();
  }
  class Component {
    constructor(props) {
      this.props = props || EMPTY;
      this.state = {};
      this.vnode = null;
    }
    setState(partial) {
      this.state = Object.assign({}, this.state, typeof partial === "function" ? partial(this.state, this.props) : partial);
      dirty.add(this);
      if (!frame) frame = requestAnimationFrame(flush);
    }
    mount(props) {
      this.props = props || this.props;
      return (this.vnode = this.render(this.props));
    }
    update() {
      if (!this.vnode || !this.vnode.el) return;
      const next = this.render(this.props);
      patch(this.vnode.el.parentNode, this.vnode, next, false);
      this.vnode = next;
    }
    render() {
      return null;
    }
  }

  class Registry {
    constructor() {
      this.entries = new Map();
    }
    register(name, value) {
      if (this.entries.has(name)) throw new Error("Duplicate registration " + name);
      this.entries.set(name, value);
      return value;
    }
    get(name) {
      return this.entries.get(name);
    }
    names() {
      return Array.from(this.entries.keys());
    }
  }

  class EventEmitter {
    constructor() {
      this.listeners = Object.create(null);
    }
    on(type, listener) {
      (this.listeners[type] || (this.listeners[type] = [])).push(listener);
      return () => this.off(type, listener);
    }
    off(type, listener) {
      const list = this.listeners[type];
      if (!list) return;
      const index = list.indexOf(listener);
      if (index >= 0) list.splice(index, 1);
    }
    emit(type, payload) {
      const list = this.listeners[type];
      if (!list) return false;
      for (const listener of list.slice()) listener(payload);
      return true;
    }
  }

  // A Redux-style store whose reducers register themselves by name.
  function createStore() {
    const reducers = Object.create(null);
    const listeners = [];
    let state = {};
    let dispatching = false;
    return {
      registerReducer(name, reducer) {
        reducers[name] = reducer;
        state = Object.assign({}, state, { [name]: reducer(undefined, { type: "@@init/" + name }) });
      },
      dispatch(action) {
        if (dispatching) throw new Error("Reducers may not dispatch actions");
        dispatching = true;
        let changed = false;
        const next = {};
        try {
          for (const name in reducers) {
            next[name] = reducers[name](state[name], action);
            if (next[name] !== state[name]) changed = true;
          }
        } finally {
          dispatching = false;
        }
        if (changed) {
          state = next;
          for (const listener of listeners.slice()) listener(state, action);
        }
        return action;
      },
      getState() {
        return state;
      },
      subscribe(listener) {
        listeners.push(listener);
        return () => {
          const index = listeners.indexOf(listener);
          if (index >= 0) listeners.splice(index, 1);
        };
      }
    };
  }

  // Remembers the result for the last arguments, compared by identity.
  function memoize(fn) {
    let lastArgs = null;
    let lastResult;
    return function () {
      if (lastArgs && lastArgs.length === arguments.length) {
        let same = true;
        for (let i = 0; i < arguments.length; i++) {
          if (lastArgs[i] !== arguments[i]) {
            same = false;
            break;
          }
        }
        if (same) return lastResult;
      }
      lastArgs = Array.prototype.slice.call(arguments);
      lastResult = fn.apply(this, arguments);
      return lastResult;
    };
  }

  const collator = new Intl.Collator("en-US", { sensitivity: "base", numeric: true });
  function compareBy(key, direction) {
    const sign = direction === "desc" ? -1 : 1;
    return function (a, b) {
      const x = a[key];
      const y = b[key];
      if (typeof x === "string" && typeof y === "string") return sign * collator.compare(x, y);
      return sign * ((x > y) - (x < y));
    };
  }

  // One listener per event type on the root; handlers are looked up by the
  // nearest data-action attribute.
  function delegate(root, type, handlers) {
    const listener = (event) => {
      let node = event.target;
      while (node && node !== root) {
        const action = node.getAttribute && node.getAttribute("data-action");
        if (action && handlers[action]) {
          handlers[action](event, node);
          return;
        }
        node = node.parentNode;
      }
    };
    root.addEventListener(type, listener);
    return () => root.removeEventListener(type, listener);
  }

  function debounce(fn, wait) {
    let timer = 0;
    return function () {
      const args = arguments;
      const self = this;
      clearTimeout(timer);
      timer = setTimeout(() => fn.apply(self, args), wait);
    };
  }

  function throttle(fn, wait) {
    let last = 0;
    let timer = 0;
    return function () {
      const now = Date.now();
      const args = arguments;
      const self = this;
      if (now - last >= wait) {
        last = now;
        fn.apply(self, args);
      } else if (!timer) {
        timer = setTimeout(() => {
          timer = 0;
          last = Date.now();
          fn.apply(self, args);
        }, wait - (now - last));
      }
    };
  }

  function classNames() {
    const out = [];
    for (let i = 0; i < arguments.length; i++) {
      const value = arguments[i];
      if (!value) continue;
      if (typeof value === "string") out.push(value);
      else for (const name in value) if (value[name]) out.push(name);
    }
    return out.join(" ");
  }

  const dateFormat = new Intl.DateTimeFormat("en-US", { month: "short", day: "numeric", year: "numeric", timeZone: "UTC" });
  const relativeFormat = typeof Intl.RelativeTimeFormat === "function" ? new Intl.RelativeTimeFormat("en-US", { numeric: "auto" }) : null;
  const numberFormat = new Intl.NumberFormat("en-US");
  const UNITS = [
    ["year", 31536000000],
    ["month", 2592000000],
    ["week", 604800000],
    ["day", 86400000],
    ["hour", 3600000],
    ["minute", 60000]
  ];
  function formatDate(value) {
    return dateFormat.format(new Date(value));
  }
  function formatRelative(value, now) {
    const diff = new Date(value).getTime() - now;
    for (const [unit, ms] of UNITS) {
      if (Math.abs(diff) >= ms || unit === "minute") {
        const amount = Math.round(diff / ms);
        return relativeFormat ? relativeFormat.format(amount, unit) : Math.abs(amount) + " " + unit + "s ago";
      }
    }
    return "";
  }
  function formatNumber(value) {
    return numberFormat.format(value);
  }
  function initials(name) {
    const parts = String(name).split(/\s+/);
    return (parts[0][0] + (parts.length > 1 ? parts[parts.length - 1][0] : "")).toUpperCase();
  }

  function delay(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
  }
  // Resolves once the page's first contentful paint is on screen, or after
  // timeout ms if that never comes.
  function firstContentfulPaint(timeout) {
    return new Promise((resolve) => {
      const painted = () => performance.getEntriesByName("first-contentful-paint").length > 0;
      if (painted()) return resolve();
      const timer = setTimeout(resolve, timeout);
      try {
        const observer = new PerformanceObserver(() => {
          if (!painted()) return;
          observer.disconnect();
          clearTimeout(timer);
          resolve();
        });
        observer.observe({ type: "paint", buffered: true });
      } catch (error) {
        // No paint timing: wait for the timeout.
      }
    });
  }
  function loadJSON(url) {
    return fetch(url).then((response) => {
      if (!response.ok && response.status !== 0) throw new Error("Failed to load " + url + ": " + response.status);
      return response.json();
    });
  }

  module.exports = {
    h,
    text,
    render,
    patch,
    Component,
    EventEmitter,
    registry: new Registry(),
    store: createStore(),
    memoize,
    compareBy,
    delegate,
    debounce,
    throttle,
    classNames,
    formatDate,
    formatRelative,
    formatNumber,
    initials,
    delay,
    firstContentfulPaint,
    loadJSON
  };
});
`;

// A general-purpose helper module, the kind a bundle carries many of.
function helperModule(random: Random, name: string) {
  const table = Array.from({ length: 16 }, () => random.int(0, 0xffff)).join(', ');
  const units = Array.from({ length: random.int(3, 6) }, () => random.pick(NOUNS).slice(0, random.int(1, 3)));
  const factors = units.map(() => random.int(2, 1000));
  return `
__define("lib/${name}", function (module, exports) {
  "use strict";
  const TABLE = [${table}];
  const UNITS = ${JSON.stringify(Object.fromEntries(units.map((unit, i) => [unit, factors[i]])))};
  function hash(input) {
    let value = ${random.int(1, 0x7fffffff)};
    for (let i = 0; i < input.length; i++) {
      value = Math.imul(value ^ input.charCodeAt(i), ${random.pick([16777619, 2654435761, 1597334677, 3812015801])}) >>> 0;
      value ^= TABLE[value & 15];
    }
    return value >>> 0;
  }
  function chunk(list, size) {
    const out = [];
    for (let i = 0; i < list.length; i += size) out.push(list.slice(i, i + size));
    return out;
  }
  function groupBy(list, key) {
    const groups = new Map();
    for (const entry of list) {
      const group = typeof key === "function" ? key(entry) : entry[key];
      if (!groups.has(group)) groups.set(group, []);
      groups.get(group).push(entry);
    }
    return groups;
  }
  function clamp(value, min = ${random.int(0, 10)}, max = ${random.int(100, 10000)}) {
    return value < min ? min : value > max ? max : value;
  }
  function format(value, unit) {
    const factor = UNITS[unit];
    if (!factor) return String(value);
    const scaled = value / factor;
    return (scaled >= 100 ? Math.round(scaled) : scaled.toFixed(${random.int(1, 2)})) + " " + unit;
  }
  function parse(input) {
    const match = /^\\s*(-?\\d+(?:\\.\\d+)?)\\s*([a-z]+)?\\s*$/i.exec(input);
    if (!match) return NaN;
    const factor = match[2] ? UNITS[match[2].toLowerCase()] : 1;
    return factor ? parseFloat(match[1]) * factor : NaN;
  }
  function pick(source, keys) {
    const out = {};
    for (const key of keys) if (Object.prototype.hasOwnProperty.call(source, key)) out[key] = source[key];
    return out;
  }
  function diff(a, b) {
    const added = [];
    const removed = [];
    const seen = new Set(a);
    for (const value of b) if (!seen.has(value)) added.push(value);
    const next = new Set(b);
    for (const value of a) if (!next.has(value)) removed.push(value);
    return { added, removed };
  }
  module.exports = { hash, chunk, groupBy, clamp, format, parse, pick, diff };
});
`;
}

function schemaModule(random: Random, name: string, fields?: Record<string, object>) {
  const generated: Record<string, object> = fields ?? {};
  if (!fields) {
    generated.id = { type: 'string', required: true, pattern: '^[A-Za-z0-9-]+$' };
    for (let n = random.int(6, 12); n > 0; n--) {
      const type = random.pick(['string', 'number', 'boolean', 'array']);
      const field: Record<string, unknown> = { type, required: random.bool(0.4) };
      if (type === 'string') field.maxLength = random.int(16, 512);
      if (type === 'number') Object.assign(field, { min: 0, max: random.int(10, 100000) });
      generated[camel([random.pick(NOUNS), capitalize(random.pick(NOUNS))])] = field;
    }
  }
  return `
__define("schema/${name}", function (module, exports) {
  "use strict";
  const FIELDS = ${JSON.stringify(generated)};
  const PATTERNS = {};
  for (const key in FIELDS) if (FIELDS[key].pattern) PATTERNS[key] = new RegExp(FIELDS[key].pattern);
  function typeOf(value) {
    return Array.isArray(value) ? "array" : value === null ? "null" : typeof value;
  }
  function validate(value) {
    const errors = [];
    if (typeOf(value) !== "object") return [{ path: "", message: "expected an object" }];
    for (const key in FIELDS) {
      const field = FIELDS[key];
      const actual = value[key];
      if (actual === undefined || actual === null) {
        if (field.required) errors.push({ path: key, message: "is required" });
        continue;
      }
      const type = typeOf(actual);
      if (type !== field.type) {
        errors.push({ path: key, message: "expected " + field.type + ", got " + type });
        continue;
      }
      if (field.maxLength !== undefined && actual.length > field.maxLength) errors.push({ path: key, message: "is too long" });
      if (field.min !== undefined && actual < field.min) errors.push({ path: key, message: "is below " + field.min });
      if (field.max !== undefined && actual > field.max) errors.push({ path: key, message: "is above " + field.max });
      if (PATTERNS[key] && !PATTERNS[key].test(actual)) errors.push({ path: key, message: "has an invalid format" });
      if (field.enum && field.enum.indexOf(actual) < 0) errors.push({ path: key, message: "is not one of " + field.enum.join(", ") });
    }
    return errors;
  }
  module.exports = { FIELDS, validate };
});
`;
}

// ---------------------------------------------------------------------------
// scripts/locale.js

function localeScript(random: Random, components: string[]) {
  const messages: Record<string, string> = {};
  for (const status of STATUSES) messages[`status.${status}`] = capitalize(status.replace('_', ' '));
  for (const priority of PRIORITIES) messages[`priority.${priority}`] = capitalize(priority);
  Object.assign(messages, {
    'app.name': 'Tasks',
    'app.loading': 'Loading your workspace…',
    'search.placeholder': 'Search items, people and projects',
    'sidebar.views': 'Views',
    'sidebar.projects': 'Projects',
    'sidebar.labels': 'Labels',
    'toolbar.new': 'New item',
    'toolbar.filter': 'Filter',
    'toolbar.sort': 'Sort',
    'table.id': 'ID',
    'table.title': 'Title',
    'table.status': 'Status',
    'table.priority': 'Priority',
    'table.assignee': 'Assignee',
    'table.updated': 'Updated',
    'table.progress': 'Progress',
    'table.unassigned': 'Unassigned',
    'summary.title': 'All items in {workspace}',
    'summary.text':
      '{open, plural, one {# item is} other {# items are}} still open across {projects} projects, ' +
      '{blocked, plural, one {# of them is} other {# of them are}} blocked and {urgent} need attention today. ' +
      '{done, plural, one {# item was} other {# items were}} finished, and {people} people updated ' +
      'something in the last month. The busiest project is {busiest}, with {busiestCount} open items; ' +
      'most recent activity was {recent}.',
    'stats.open': 'Open',
    'stats.done': 'Done',
    'stats.blocked': 'Blocked',
    'stats.points': 'Open points',
    'statusbar.items': '{count, plural, one {# item} other {# items}}',
    'statusbar.synced': 'Synced {when}'
  });
  for (const view of ['inbox', 'my_items', 'starred', 'recent', 'created', 'subscribed', 'drafts', 'archive']) {
    messages[`view.${view}`] = capitalize(view.replace('_', ' '));
  }
  for (const name of components) {
    const words = kebab(name).split('-');
    messages[`${name}.title`] = capitalize(words.join(' '));
    messages[`${name}.empty`] = `No ${words[0]}s match the current filter.`;
    messages[`${name}.count`] = `{count, plural, one {# ${words[0]}} other {# ${words[0]}s}}`;
    messages[`${name}.more`] = 'Show {count} more';
    messages[`${name}.less`] = 'Show less';
    messages[`${name}.description`] = sentence(random, 8, 16);
    messages[`${name}.error`] = `Could not load ${words.join(' ')}: {reason}`;
  }
  return `__define("i18n", function (module, exports) {
  "use strict";
  const MESSAGES = ${JSON.stringify(messages, null, 2)};
  const plurals = new Intl.PluralRules("en-US");
  const numbers = new Intl.NumberFormat("en-US");
  const cache = new Map();
  // Compiles "{name}" and "{name, plural, one {...} other {...}}" into parts.
  function compile(message) {
    const parts = [];
    let i = 0;
    while (i < message.length) {
      const open = message.indexOf("{", i);
      if (open < 0) {
        parts.push(message.slice(i));
        break;
      }
      if (open > i) parts.push(message.slice(i, open));
      let depth = 1;
      let j = open + 1;
      while (j < message.length && depth > 0) {
        if (message[j] === "{") depth++;
        else if (message[j] === "}") depth--;
        j++;
      }
      const body = message.slice(open + 1, j - 1);
      const comma = body.indexOf(",");
      if (comma < 0) {
        parts.push({ name: body.trim() });
      } else {
        const name = body.slice(0, comma).trim();
        const rest = body.slice(body.indexOf(",", comma + 1) + 1);
        const options = {};
        const re = /\\s*(=?\\w+)\\s*\\{((?:[^{}]|\\{[^{}]*\\})*)\\}/g;
        let match;
        while ((match = re.exec(rest))) options[match[1]] = compile(match[2]);
        parts.push({ name, plural: options });
      }
      i = j;
    }
    return parts;
  }
  function format(parts, params, count) {
    let out = "";
    for (const part of parts) {
      if (typeof part === "string") {
        out += count === undefined ? part : part.replace(/#/g, numbers.format(count));
      } else if (part.plural) {
        const value = params[part.name];
        const branch = part.plural["=" + value] || part.plural[plurals.select(value)] || part.plural.other;
        out += format(branch, params, value);
      } else {
        const value = params[part.name];
        out += typeof value === "number" ? numbers.format(value) : value == null ? "" : String(value);
      }
    }
    return out;
  }
  function t(key, params) {
    let parts = cache.get(key);
    if (!parts) {
      const message = MESSAGES[key];
      if (message === undefined) return key;
      parts = compile(message);
      cache.set(key, parts);
    }
    return format(parts, params || {});
  }
  module.exports = { t, has: (key) => key in MESSAGES };
});
`;
}

// ---------------------------------------------------------------------------
// scripts/app.js: feature modules

const ITEM_FIELDS = ['progress', 'estimate', 'comments'];

function componentModule(random: Random, name: string) {
  const Name = capitalize(name);
  const cls = `c-${kebab(name)}`;
  const field = random.pick(ITEM_FIELDS);
  const op = random.pick(['>', '>=', '<', '<=']);
  const threshold =
    field === 'progress' ? random.int(1, 19) * 5 : field === 'estimate' ? random.pick([2, 3, 5, 8]) : random.int(2, 30);
  const sortKey = random.pick(['updated', 'title', 'estimate', 'comments']);
  const statuses = STATUSES.filter(() => random.bool(0.6));
  return `
__define("components/${name}", function (module, exports, require) {
  "use strict";
  const core = require("core");
  const i18n = require("i18n");
  const h = core.h;
  const DEFAULTS = Object.freeze({ density: ${JSON.stringify(random.pick(['compact', 'comfortable', 'cozy']))}, limit: ${random.int(4, 8)}, sortKey: ${JSON.stringify(sortKey)}, direction: ${JSON.stringify(random.pick(['asc', 'desc']))} });
  const STATUSES = new Set(${JSON.stringify(statuses)});
  function matches(item) {
    return item.${field} ${op} ${threshold} && (STATUSES.size === 0 || STATUSES.has(item.status));
  }
  function describe(item, options) {
    const parts = [item.id, item.title];
    if (options.density !== "compact") parts.push("(" + i18n.t("status." + item.status) + ")");
    return parts.join(" ");
  }
  class ${Name} extends core.Component {
    constructor(props) {
      super(props);
      this.options = Object.assign({}, DEFAULTS, props && props.options);
      this.state = { expanded: ${random.bool()}, page: 0 };
      this.compare = core.compareBy(this.options.sortKey, this.options.direction);
      this.select = core.memoize((items) => items.filter(matches).sort(this.compare));
    }
    toggle() {
      this.setState((state) => ({ expanded: !state.expanded, page: 0 }));
    }
    visible(items) {
      const all = this.select(items);
      const limit = this.state.expanded ? this.options.limit * 3 : this.options.limit;
      return { all, shown: all.slice(this.state.page * limit, (this.state.page + 1) * limit) };
    }
    render(props) {
      const { all, shown } = this.visible(props.items || []);
      return h(
        "section",
        { class: core.classNames("${cls}", { "${cls}--active": this.state.expanded, "${cls}--compact": this.options.density === "compact" }), key: "${name}" },
        h("header", { class: "${cls}__header" }, h("h3", null, i18n.t("${name}.title")), h("button", { class: "${cls}__toggle", "data-action": "component/toggle", "data-component": "${name}" }, i18n.t(this.state.expanded ? "${name}.less" : "${name}.more", { count: Math.max(0, all.length - shown.length) }))),
        shown.length
          ? h("ul", { class: "${cls}__body" }, shown.map((item) => h("li", { key: item.id, title: item.title }, describe(item, this.options))))
          : h("p", { class: "${cls}__footer" }, i18n.t("${name}.empty")),
        h("footer", { class: "${cls}__footer" }, i18n.t("${name}.count", { count: all.length }))
      );
    }
  }
  core.registry.register("${name}", ${Name});
  module.exports = { ${Name}, matches, describe };
});
`;
}

function reducerModule(random: Random, name: string) {
  const field = random.pick(ITEM_FIELDS);
  const threshold = field === 'progress' ? random.int(1, 19) * 5 : random.int(2, 20);
  const pageSize = random.pick([20, 25, 50, 100]);
  const derives = random.bool(0.5);
  return `
__define("store/${name}", function (module, exports, require) {
  "use strict";
  const core = require("core");
  const PAGE_SIZE = ${pageSize};
  const initialState = Object.freeze({ ids: [], byId: {}, selected: null, loading: false, error: null, page: 0, matching: 0, filter: ${JSON.stringify(random.pick(['all', 'open', 'mine', 'recent']))} });
  function reducer(state = initialState, action) {
    switch (action.type) {
      case "${name}/request":
        return Object.assign({}, state, { loading: true, error: null });
      case "${name}/receive": {
        const byId = Object.assign({}, state.byId);
        const ids = state.ids.slice();
        for (const entry of action.payload) {
          if (!byId[entry.id]) ids.push(entry.id);
          byId[entry.id] = Object.assign({}, byId[entry.id], entry);
        }
        return Object.assign({}, state, { byId, ids, loading: false });
      }
      case "${name}/fail":
        return Object.assign({}, state, { loading: false, error: String(action.error) });
      case "${name}/select":
        return state.selected === action.id ? state : Object.assign({}, state, { selected: action.id });
      case "${name}/page": {
        const last = Math.max(0, Math.ceil(state.ids.length / PAGE_SIZE) - 1);
        return Object.assign({}, state, { page: Math.max(0, Math.min(action.page, last)) });
      }
      case "${name}/filter":
        return state.filter === action.filter ? state : Object.assign({}, state, { filter: action.filter, page: 0 });${
          derives
            ? `
      case "items/loaded": {
        let matching = 0;
        for (const item of action.data.items) if (item.${field} >= ${threshold}) matching++;
        return matching === state.matching ? state : Object.assign({}, state, { matching });
      }`
            : ''
        }
      default:
        return state;
    }
  }
  const selectPage = core.memoize((state) => state.ids.slice(state.page * PAGE_SIZE, (state.page + 1) * PAGE_SIZE).map((id) => state.byId[id]));
  const selectSelected = (state) => (state.selected ? state.byId[state.selected] : null);
  core.store.registerReducer("${name}", reducer);
  module.exports = { reducer, selectPage, selectSelected, PAGE_SIZE };
});
`;
}

function serviceModule(random: Random, name: string) {
  const Name = `${capitalize(name)}Service`;
  const delays = Array.from({ length: random.int(2, 4) }, (_, i) => 100 * 2 ** i + random.int(0, 50));
  return `
__define("services/${name}", function (module, exports, require) {
  "use strict";
  const core = require("core");
  const RETRY_DELAYS = ${JSON.stringify(delays)};
  class ${Name} extends core.EventEmitter {
    constructor(options) {
      super();
      options = options || {};
      this.endpoint = options.endpoint || "api/${kebab(name)}";
      this.maxEntries = options.maxEntries || ${random.int(16, 256)};
      this.cache = new Map();
      this.pending = new Map();
    }
    key(params) {
      return Object.keys(params)
        .sort()
        .map((name) => name + "=" + encodeURIComponent(params[name]))
        .join("&");
    }
    get(params) {
      const key = this.key(params || {});
      if (this.cache.has(key)) return Promise.resolve(this.cache.get(key));
      if (this.pending.has(key)) return this.pending.get(key);
      const request = this.fetchWithRetry(this.endpoint + "?" + key, 0).then(
        (value) => {
          this.pending.delete(key);
          this.remember(key, value);
          this.emit("loaded", { key, value });
          return value;
        },
        (error) => {
          this.pending.delete(key);
          this.emit("error", { key, error });
          throw error;
        }
      );
      this.pending.set(key, request);
      return request;
    }
    fetchWithRetry(url, attempt) {
      return core.loadJSON(url).catch((error) => {
        if (attempt >= RETRY_DELAYS.length) throw error;
        return core.delay(RETRY_DELAYS[attempt]).then(() => this.fetchWithRetry(url, attempt + 1));
      });
    }
    remember(key, value) {
      this.cache.delete(key);
      this.cache.set(key, value);
      while (this.cache.size > this.maxEntries) this.cache.delete(this.cache.keys().next().value);
    }
    invalidate(prefix) {
      for (const key of Array.from(this.cache.keys())) if (key.indexOf(prefix || "") === 0) this.cache.delete(key);
      this.emit("invalidated", prefix);
    }
  }
  module.exports = { ${Name}, instance: new ${Name}() };
});
`;
}

// The app's own views and bootstrap, which require everything else.
function mainModule(icons: Record<string, string[]>, eager: string[], insights: string[]) {
  return String.raw`
__define("app/icons", function (module) {
  "use strict";
  const h = __require("core").h;
  const PATHS = ${JSON.stringify(icons)};
  module.exports = function icon(name, extra) {
    return h("svg", { class: "icon" + (extra ? " " + extra : ""), viewBox: "0 0 24 24", "aria-hidden": "true" }, (PATHS[name] || PATHS.doc).map((d) => h("path", { d })));
  };
});

__define("store/items", function (module, exports, require) {
  "use strict";
  const core = require("core");
  const empty = Object.freeze({ loaded: false, ids: [], byId: {}, users: {}, projects: {}, labels: {}, projectOrder: [], labelOrder: [], views: [], counts: null, now: 0, user: null, workspace: "" });
  function countBy(items, key) {
    const counts = Object.create(null);
    for (const item of items) counts[item[key]] = (counts[item[key]] || 0) + 1;
    return counts;
  }
  function index(list) {
    const out = {};
    for (const entry of list) out[entry.id] = entry;
    return out;
  }
  function reducer(state = empty, action) {
    switch (action.type) {
      case "items/loaded": {
        const data = action.data;
        const items = data.items.map((item) => Object.assign({}, item, { updatedAt: Date.parse(item.updated) }));
        items.sort(core.compareBy("updatedAt", "desc"));
        const open = items.filter((item) => item.status !== "done" && item.status !== "canceled");
        return {
          loaded: true,
          ids: items.map((item) => item.id),
          byId: index(items),
          users: index(data.users),
          projects: index(data.projects),
          labels: index(data.labels),
          projectOrder: data.projects.map((project) => project.id),
          labelOrder: data.labels.map((label) => label.id),
          views: data.views,
          counts: {
            status: countBy(items, "status"),
            priority: countBy(items, "priority"),
            openByProject: countBy(open, "project"),
            open: open.length,
            points: open.reduce((sum, item) => sum + item.estimate, 0),
            people: new Set(items.filter((item) => Date.parse(data.now) - item.updatedAt < 2592000000).map((item) => item.assignee)).size
          },
          now: Date.parse(data.now),
          user: data.user,
          workspace: data.projects[0].name.split(" ")[0] + " workspace"
        };
      }
      default:
        return state;
    }
  }
  core.store.registerReducer("items", reducer);
  module.exports = { reducer };
});

__define("store/ui", function (module, exports, require) {
  "use strict";
  const core = require("core");
  const initial = Object.freeze({ view: "inbox", project: null, label: null, query: "", selected: null, sort: "updated" });
  function reducer(state = initial, action) {
    switch (action.type) {
      case "ui/view":
        return Object.assign({}, state, { view: action.view, project: null, label: null });
      case "ui/project":
        return Object.assign({}, state, { project: action.project, label: null });
      case "ui/label":
        return Object.assign({}, state, { label: state.label === action.label ? null : action.label });
      case "ui/query":
        return Object.assign({}, state, { query: action.query });
      case "ui/select":
        return Object.assign({}, state, { selected: action.id });
      default:
        return state;
    }
  }
  core.store.registerReducer("ui", reducer);
  module.exports = { reducer };
});

__define("app/views", function (module, exports, require) {
  "use strict";
  const core = require("core");
  const i18n = require("i18n");
  const icon = require("app/icons");
  const h = core.h;
  const VIEW_ICONS = { inbox: "inbox", my_items: "user", starred: "star", recent: "clock", created: "plus", subscribed: "bell", drafts: "doc", archive: "archive" };
  const STATUS_ICONS = { backlog: "layers", todo: "doc", in_progress: "clock", in_review: "check", blocked: "alert", done: "check", canceled: "archive" };
  const hue = (hueValue, lightness) => "hsl(" + hueValue + ", 60%, " + lightness + "%)";

  function Titlebar(items) {
    return h("header", { class: "titlebar", key: "titlebar" },
      icon("layers"),
      h("span", { class: "titlebar__name" }, i18n.t("app.name")),
      items.loaded ? h("span", { class: "text-12" }, items.workspace) : null,
      h("input", { class: "titlebar__search", type: "search", placeholder: i18n.t("search.placeholder"), "data-action": "search" }),
      icon("bell"),
      icon("settings"));
  }

  function NavItem(props) {
    return h("li", { key: props.key },
      h("button", { class: core.classNames("nav-item", { "nav-item--active": props.active }), "data-action": props.action, "data-value": props.value },
        props.icon,
        h("span", { class: "nav-item__label" }, props.label),
        props.count != null ? h("span", { class: "nav-item__count" }, core.formatNumber(props.count)) : null));
  }

  function Sidebar(items, ui) {
    const counts = items.counts;
    return h("nav", { class: "sidebar", key: "sidebar" },
      h("section", { class: "sidebar__section" },
        h("h2", { class: "sidebar__heading" }, i18n.t("sidebar.views"), icon("plus", "icon--sm")),
        h("ul", null, (items.loaded ? items.views : ["inbox", "my_items", "starred", "recent"]).map((view) =>
          NavItem({ key: view, action: "nav/view", value: view, active: ui.view === view && !ui.project, icon: icon(VIEW_ICONS[view]), label: i18n.t("view." + view), count: view === "inbox" && counts ? counts.open : null })))),
      items.loaded
        ? h("section", { class: "sidebar__section" },
            h("h2", { class: "sidebar__heading" }, i18n.t("sidebar.projects"), icon("plus", "icon--sm")),
            h("ul", null, items.projectOrder.map((id) => {
              const project = items.projects[id];
              return NavItem({ key: id, action: "nav/project", value: id, active: ui.project === id, icon: h("span", { class: "nav-item__dot", style: { background: hue(project.hue, 50) } }), label: project.name, count: counts.openByProject[id] || 0 });
            })))
        : null,
      items.loaded
        ? h("section", { class: "sidebar__section" },
            h("h2", { class: "sidebar__heading" }, i18n.t("sidebar.labels")),
            h("div", { class: "label-cloud" }, items.labelOrder.map((id) => Label(items.labels[id], ui.label === id))))
        : null);
  }

  function Label(label, active) {
    return h("span", { key: label.id, class: core.classNames("label", { "chip--active": active }), style: { background: hue(label.hue, 92), color: hue(label.hue, 30) }, "data-action": "nav/label", "data-value": label.id }, label.name);
  }

  function Toolbar(items, ui) {
    const title = ui.project && items.loaded ? items.projects[ui.project].name : i18n.t("view." + ui.view);
    return h("div", { class: "toolbar", key: "toolbar" },
      h("span", { class: "toolbar__title" }, title),
      ["all", "open", "mine"].map((filter) => h("button", { key: filter, class: core.classNames("chip", { "chip--active": filter === "all" }), "data-action": "filter", "data-value": filter }, filter)),
      h("span", { class: "toolbar__spacer" }),
      h("button", { class: "button", "data-action": "toolbar/filter" }, icon("filter"), i18n.t("toolbar.filter")),
      h("button", { class: "button", "data-action": "toolbar/sort" }, icon("sort"), i18n.t("toolbar.sort")),
      h("button", { class: "button button--primary", "data-action": "toolbar/new" }, icon("plus"), i18n.t("toolbar.new")));
  }

  function Summary(items) {
    const counts = items.counts;
    let busiest = items.projectOrder[0];
    for (const id of items.projectOrder) if ((counts.openByProject[id] || 0) > (counts.openByProject[busiest] || 0)) busiest = id;
    const latest = items.byId[items.ids[0]];
    const stats = [
      ["stats.open", counts.open],
      ["stats.done", counts.status.done || 0],
      ["stats.blocked", counts.status.blocked || 0],
      ["stats.points", counts.points]
    ];
    return h("section", { class: "summary", key: "summary" },
      h("div", null,
        h("h1", { class: "summary__title" }, i18n.t("summary.title", { workspace: items.workspace })),
        h("p", { class: "summary__text" }, i18n.t("summary.text", {
          open: counts.open,
          projects: items.projectOrder.length,
          blocked: counts.status.blocked || 0,
          urgent: counts.priority.urgent || 0,
          done: counts.status.done || 0,
          people: counts.people,
          busiest: items.projects[busiest].name,
          busiestCount: counts.openByProject[busiest] || 0,
          recent: core.formatRelative(latest.updatedAt, items.now) + " on " + latest.id
        }))),
      h("div", { class: "summary__stats" }, stats.map((stat) =>
        h("div", { class: "stat", key: stat[0] }, h("div", { class: "stat__value" }, core.formatNumber(stat[1])), h("div", { class: "stat__label" }, i18n.t(stat[0]))))));
  }

  // A few registered feature components, rendered from the store.
  const INSIGHTS = ${JSON.stringify(insights)};
  const insightInstances = new Map();
  function Insights(items) {
    const list = items.ids.map((id) => items.byId[id]);
    return h("div", { class: "insights", key: "insights" }, INSIGHTS.map((name) => {
      let instance = insightInstances.get(name);
      if (!instance) {
        const Component = core.registry.get(name);
        instance = new Component({ items: list });
        insightInstances.set(name, instance);
      }
      instance.props = { items: list };
      return instance.mount();
    }));
  }

  function Row(item, items, selected) {
    const user = item.assignee ? items.users[item.assignee] : null;
    const project = items.projects[item.project];
    return h("div", { key: item.id, class: selected ? "row is-selected" : "row", role: "row", "data-action": "row/select", "data-id": item.id },
      h("div", { class: "cell col-icon", style: { color: hue(project.hue, 45) } }, icon(STATUS_ICONS[item.status])),
      h("div", { class: "cell col-id" }, item.id),
      h("div", { class: "cell col-title" },
        h("span", { class: "item-title" }, item.title),
        item.labels.length ? h("span", { class: "item-labels" }, item.labels.map((id) => Label(items.labels[id], false))) : null),
      h("div", { class: "cell col-status" }, h("span", { class: "status status--" + item.status }, i18n.t("status." + item.status))),
      h("div", { class: "cell col-priority" }, h("span", { class: "priority priority--" + item.priority }, i18n.t("priority." + item.priority))),
      h("div", { class: "cell col-assignee" }, user
        ? [h("span", { class: "avatar", style: { background: hue((user.id.length * 53 + user.name.charCodeAt(0) * 7) % 360, 45) } }, core.initials(user.name)), user.name]
        : h("span", { class: "text-muted" }, i18n.t("table.unassigned"))),
      h("div", { class: "cell col-updated", title: core.formatDate(item.updatedAt) }, core.formatRelative(item.updatedAt, items.now)),
      h("div", { class: "cell col-progress" }, h("div", { class: "progress" }, h("div", { class: "progress__bar", style: { width: item.progress + "%" } }))));
  }

  const COLUMNS = [["col-icon", ""], ["col-id", "table.id"], ["col-title", "table.title"], ["col-status", "table.status"], ["col-priority", "table.priority"], ["col-assignee", "table.assignee"], ["col-updated", "table.updated"], ["col-progress", "table.progress"]];
  function ItemTable(items, ui) {
    return h("div", { class: "table-wrap", key: "table" },
      h("div", { class: "items", role: "grid", "aria-rowcount": items.ids.length },
        h("div", { class: "items__head", role: "row" }, COLUMNS.map((column) => h("div", { key: column[0], class: "cell " + column[0], role: "columnheader" }, column[1] ? i18n.t(column[1]) : ""))),
        h("div", { class: "items__body", role: "rowgroup" }, items.ids.map((id) => Row(items.byId[id], items, ui.selected === id)))));
  }

  function Main(items, ui) {
    if (!items.loaded) return h("main", { class: "main", key: "main" }, h("p", { class: "main__loading" }, i18n.t("app.loading")));
    return h("main", { class: "main", key: "main" }, Summary(items), Insights(items), ItemTable(items, ui));
  }

  function StatusBar(items) {
    return h("footer", { class: "statusbar", key: "statusbar" },
      h("span", null, items.loaded ? i18n.t("statusbar.items", { count: items.ids.length }) : ""),
      h("span", null, items.loaded ? i18n.t("statusbar.synced", { when: core.formatRelative(items.now - 60000, items.now) }) : ""));
  }

  function App(state) {
    return h("div", { class: "app" }, Titlebar(state.items), Sidebar(state.items, state.ui), Toolbar(state.items, state.ui), Main(state.items, state.ui), StatusBar(state.items));
  }
  module.exports = { App };
});

__define("app/main", function (module, exports, require) {
  "use strict";
  // Start loading the data first, as an app would, so it overlaps with
  // evaluating the modules.
  const core = require("core");
  const data = core.loadJSON("data/items.json");
  const EAGER = ${JSON.stringify(eager)};
  for (const id of EAGER) require(id);
  require("store/items");
  require("store/ui");
  const itemSchema = require("schema/item");
  const views = require("app/views");
  const store = core.store;
  const root = document.getElementById("app");

  let current = null;
  let scheduled = 0;
  function renderNow() {
    scheduled = 0;
    current = core.render(root, views.App(store.getState()), current);
  }
  store.subscribe(() => {
    if (!scheduled) scheduled = requestAnimationFrame(renderNow);
  });

  const handlers = {
    "nav/view": (event, node) => store.dispatch({ type: "ui/view", view: node.getAttribute("data-value") }),
    "nav/project": (event, node) => store.dispatch({ type: "ui/project", project: node.getAttribute("data-value") }),
    "nav/label": (event, node) => store.dispatch({ type: "ui/label", label: node.getAttribute("data-value") }),
    "row/select": (event, node) => store.dispatch({ type: "ui/select", id: node.getAttribute("data-id") }),
    "component/toggle": (event, node) => {
      const Component = core.registry.get(node.getAttribute("data-component"));
      if (Component) store.dispatch({ type: "ui/toggle", component: node.getAttribute("data-component") });
    }
  };
  core.delegate(root, "click", handlers);
  core.delegate(root, "input", { search: core.debounce((event) => store.dispatch({ type: "ui/query", query: event.target.value }), 150) });
  window.addEventListener("keydown", (event) => {
    if ((event.metaKey || event.ctrlKey) && event.key === "k") {
      event.preventDefault();
      const search = root.querySelector(".titlebar__search");
      if (search) search.focus();
    }
  });
  let rowsInView = 0;
  if (typeof ResizeObserver === "function") {
    new ResizeObserver((entries) => {
      rowsInView = Math.ceil(entries[0].contentRect.height / 34);
    }).observe(root);
  }

  // The window chrome first, then the content once the chrome is on screen
  // and the data is in. Waiting for the chrome's paint also keeps the two
  // paints apart: when the content was rendered before the chrome's frame
  // was presented, Electron 15 and 38 (in testing) sometimes credited the
  // content's LCP to the chrome's frame.
  renderNow();
  performance.mark("app-shell-rendered");
  Promise.all([data, core.firstContentfulPaint(2000)])
    .then(([payload]) => {
      let invalid = 0;
      for (const item of payload.items) if (itemSchema.validate(item).length) invalid++;
      if (invalid) console.warn(invalid + " items failed validation");
      store.dispatch({ type: "items/loaded", data: payload });
      if (scheduled) cancelAnimationFrame(scheduled);
      renderNow();
      performance.mark("app-ready");
      if (!rowsInView) console.warn("The window has no height");
    })
    .catch((error) => {
      console.error(error);
      performance.mark("app-failed");
    });
});

__require("app/main");
`;
}

// ---------------------------------------------------------------------------

const LCP_RECORDER = `(function () {
  // crossbench's performance.entries probe reads marks, not LCP entries, so
  // the latest LCP candidate is kept as the "app-lcp" mark.
  if (typeof PerformanceObserver !== "function") return;
  try {
    new PerformanceObserver(function (list) {
      var entries = list.getEntries();
      var latest = entries[entries.length - 1];
      performance.clearMarks("app-lcp");
      performance.mark("app-lcp", { startTime: latest.startTime });
    }).observe({ type: "largest-contentful-paint", buffered: true });
  } catch (error) {
    // No LCP support: the story reports no lcp.
  }
})();`;

function indexHtml() {
  return `<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Tasks</title>
<link rel="stylesheet" href="styles/base.css">
<link rel="stylesheet" href="styles/layout.css">
<link rel="stylesheet" href="styles/components.css">
<script>
${LCP_RECORDER}
</script>
<script defer src="scripts/vendor.js"></script>
<script defer src="scripts/locale.js"></script>
<script defer src="scripts/app.js"></script>
</head>
<body>
<div id="app"></div>
</body>
</html>
`;
}

// Writes the app to <root> and returns the size of each file in bytes.
export function writeAppShellSite(root: string, { rows = 1200, seed = 0x5eed }: AppShellOptions = {}) {
  const random = createRandom(seed);
  const components = featureNames(random, 64, COMPONENT_SUFFIXES);
  const reducers = featureNames(random, 32, ['']).map((name) => `${name}s`);
  const services = featureNames(random, 20, ['']);
  const helpers = featureNames(random, 28, ['Utils', 'Helpers', 'Math', 'Format']);
  const schemas = featureNames(random, 16, ['']);
  const icons = generateIcons(random);
  const data = generateData(random, rows);

  // Every reducer and about two thirds of the rest run at startup; the rest
  // is code the first window never needs.
  const eager = [
    ...reducers.map((name) => `store/${name}`),
    ...components.filter((_, i) => i % 3 !== 2).map((name) => `components/${name}`),
    ...services.filter((_, i) => i % 2 === 0).map((name) => `services/${name}`),
    ...helpers.filter((_, i) => i % 3 === 0).map((name) => `lib/${name}`),
    ...schemas.filter((_, i) => i % 4 === 0).map((name) => `schema/${name}`)
  ];
  const insights = components.filter((_, i) => i % 3 !== 2).slice(0, 4);

  const itemSchema = {
    id: { type: 'string', required: true, pattern: '^[A-Z]+\\d+-\\d+$' },
    title: { type: 'string', required: true, maxLength: 200 },
    project: { type: 'string', required: true },
    status: { type: 'string', required: true, enum: STATUSES },
    priority: { type: 'string', required: true, enum: PRIORITIES },
    assignee: { type: 'string' },
    labels: { type: 'array' },
    updated: { type: 'string', required: true },
    estimate: { type: 'number', min: 0, max: 100 },
    progress: { type: 'number', min: 0, max: 100 },
    comments: { type: 'number', min: 0 }
  };

  const banner = (name: string) =>
    `/* ${name}: generated by script/benchmarks/lib/app-shell.mts for the Electron benchmarks. */\n`;
  const files: Record<string, string> = {
    'index.html': indexHtml(),
    'styles/base.css': banner('base.css') + generateBaseCss(random).join('\n') + '\n',
    'styles/layout.css': banner('layout.css') + generateLayoutCss().join('\n') + '\n',
    'styles/components.css': banner('components.css') + generateComponentCss(random, components).join('\n') + '\n',
    'scripts/vendor.js':
      banner('vendor.js') +
      RUNTIME +
      helpers.map((name) => helperModule(random, name)).join('') +
      schemas.map((name) => schemaModule(random, name)).join('') +
      schemaModule(random, 'item', itemSchema),
    'scripts/locale.js': banner('locale.js') + localeScript(random, components),
    'scripts/app.js':
      banner('app.js') +
      reducers.map((name) => reducerModule(random, name)).join('') +
      services.map((name) => serviceModule(random, name)).join('') +
      components.map((name) => componentModule(random, name)).join('') +
      mainModule(icons, eager, insights),
    'data/items.json': JSON.stringify(data)
  };

  fs.rmSync(root, { recursive: true, force: true });
  const sizes: Record<string, number> = {};
  for (const [name, content] of Object.entries(files)) {
    const file = path.join(root, name);
    fs.mkdirSync(path.dirname(file), { recursive: true });
    fs.writeFileSync(file, content);
    sizes[name] = Buffer.byteLength(content);
  }
  return sizes;
}
