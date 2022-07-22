import { defineProperties } from '@electron/internal/common/define-properties';
import { utilityNodeModuleList } from '@electron/internal/utility_process/api/module-list';

module.exports = {};

defineProperties(module.exports, utilityNodeModuleList);
