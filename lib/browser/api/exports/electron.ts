import { defineProperties } from '@electron/internal/common/define-properties';
import { commonModuleList } from '@electron/internal/common/api/module-list';
import { browserModuleList } from '@electron/internal/browser/api/module-list';

module.exports = {};

defineProperties(module.exports, commonModuleList);
defineProperties(module.exports, browserModuleList);
