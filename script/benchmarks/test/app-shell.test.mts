// Run with: node --test script/benchmarks/test/*.test.mts
import assert from 'node:assert/strict';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import { after, describe, it } from 'node:test';
import * as vm from 'node:vm';

import { writeAppShellSite } from '../lib/app-shell.mts';
import { pageConfigs, probeConfigs } from '../lib/configs.mts';

const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'benchmarks-app-shell-test-'));
after(() => fs.rmSync(tmp, { recursive: true, force: true }));

const read = (root: string, name: string) => fs.readFileSync(path.join(root, name), 'utf8');

describe('writeAppShellSite', () => {
  const root = path.join(tmp, 'site');
  const sizes = writeAppShellSite(root);
  const kib = (prefix: string) =>
    Object.entries(sizes)
      .filter(([name]) => name.startsWith(prefix))
      .reduce((sum, [, size]) => sum + size, 0) / 1024;

  it('writes the same bytes every time', () => {
    const again = path.join(tmp, 'again');
    writeAppShellSite(again);
    for (const name of Object.keys(sizes)) assert.equal(read(again, name), read(root, name), name);
  });

  it('references every stylesheet and script from index.html', () => {
    const html = read(root, 'index.html');
    for (const name of Object.keys(sizes).filter((file) => /\.(css|js)$/.test(file))) {
      assert.ok(html.includes(`"${name}"`), `index.html does not load ${name}`);
    }
  });

  it('has an app-sized amount of code, styles and data', () => {
    assert.ok(kib('scripts/') > 300 && kib('scripts/') < 600, `${kib('scripts/')} KiB of scripts`);
    assert.ok(kib('styles/') > 50 && kib('styles/') < 150, `${kib('styles/')} KiB of styles`);
    assert.ok(kib('data/') > 100 && kib('data/') < 300, `${kib('data/')} KiB of data`);
    const rules = Object.keys(sizes)
      .filter((name) => name.endsWith('.css'))
      .reduce((sum, name) => sum + (read(root, name).match(/\{/g) ?? []).length, 0);
    assert.ok(rules > 800, `${rules} CSS rules`);
  });

  it('writes the requested number of rows', () => {
    const data = JSON.parse(read(root, 'data/items.json'));
    assert.equal(data.items.length, 1200);
    const small = path.join(tmp, 'small');
    writeAppShellSite(small, { rows: 10 });
    assert.equal(JSON.parse(read(small, 'data/items.json')).items.length, 10);
  });

  it('writes scripts that parse', () => {
    for (const name of Object.keys(sizes).filter((file) => file.endsWith('.js'))) {
      assert.doesNotThrow(() => new vm.Script(read(root, name), { filename: name }), name);
    }
    const inline = /<script>([\s\S]*?)<\/script>/.exec(read(root, 'index.html'));
    assert.ok(inline);
    assert.doesNotThrow(() => new vm.Script(inline[1]));
  });

  // Electron 15 is Chromium 94.
  it('avoids syntax and APIs newer than Chromium 94', () => {
    const newer: Record<string, [RegExp, string]> = {
      'Array.prototype.at': [/\.at\(/, '.js'],
      findLast: [/\.findLast(Index)?\(/, '.js'],
      structuredClone: [/structuredClone/, '.js'],
      'Object.hasOwn': [/Object\.hasOwn\(/, '.js'],
      'class static blocks': [/static\s*\{/, '.js'],
      ':has()': [/:has\(/, '.css'],
      'CSS nesting': [/&[\s.:[]/, '.css'],
      '@layer': [/@layer/, '.css'],
      '@container': [/@container/, '.css'],
      'color-mix()': [/color-mix\(/, '.css']
    };
    for (const name of Object.keys(sizes)) {
      const content = read(root, name);
      for (const [feature, [pattern, extension]] of Object.entries(newer)) {
        if (name.endsWith(extension)) assert.doesNotMatch(content, pattern, `${name} uses ${feature}`);
      }
    }
  });
});

describe('pageConfigs', () => {
  it('runs the app shell from file:// in suites of its own', () => {
    const configs = pageConfigs({ baseUrl: 'http://127.0.0.1:1', fileRoot: '/tmp/files', appRoot: '/tmp/app' });
    // The synthetic page's suites are unchanged, so an app shell failure
    // can't fail their crossbench runs.
    assert.deepEqual(Object.keys(configs.loading.pages), ['first_load', 'reload']);
    assert.deepEqual(Object.keys(configs.trace.pages), ['first_load']);
    assert.deepEqual(Object.keys(configs.app_shell.pages), ['first_load', 'reload']);
    assert.deepEqual(Object.keys(configs.app_shell_trace.pages), ['first_load']);
    for (const story of [configs.app_shell.pages.first_load, configs.app_shell_trace.pages.first_load]) {
      assert.equal(story[0].url, 'file:///tmp/app/index.html');
    }
    assert.ok(configs.app_shell.pages.first_load.some((action) => action.action === 'meminfo'));
    const probes = probeConfigs();
    assert.deepEqual(probes.app_shell, probes.loading);
    assert.deepEqual(probes.app_shell_trace, probes.trace);
  });
});
