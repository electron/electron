import { defineProperties } from '@electron/internal/common/define-properties';
import { moduleList } from '@electron/internal/preload_realm/api/module-list';

module.exports = {};

defineProperties(module.exports, moduleList);
