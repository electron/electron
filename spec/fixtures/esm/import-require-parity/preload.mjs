import { ipcRenderer } from 'electron/renderer';

import { compareModules } from './compare.mjs';

const problems = await compareModules(['electron', 'electron/renderer', 'electron/common'], import.meta.url);
ipcRenderer.send('parity', problems);
