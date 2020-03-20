import { defineProperties } from '@electron/internal/common/define-properties';
import { moduleList } from '@electron/internal/sandboxed_renderer/api/module-list';

module.exports = {};

defineProperties(module.exports, moduleList);
