import { browserModuleList } from '@electron/internal/browser/api/module-list';
import { commonModuleList } from '@electron/internal/common/api/module-list';
import { defineProperties } from '@electron/internal/common/define-properties';

module.exports = {};

defineProperties(module.exports, commonModuleList);
defineProperties(module.exports, browserModuleList);
