import { wrapFsWithAsar } from './fs-wrapper';

wrapFsWithAsar(require('node:fs'));
