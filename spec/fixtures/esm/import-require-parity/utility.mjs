import { compareModules } from './compare.mjs';

const problems = await compareModules(['electron', 'electron/utility'], import.meta.url);
process.parentPort.postMessage(problems);
