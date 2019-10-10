import { defineProperties } from '@electron/internal/common/define-properties'
import { commonModuleList } from '@electron/internal/common/api/module-list'
import { browserModuleList } from '@electron/internal/browser/api/module-list'

defineProperties(exports, commonModuleList)
defineProperties(exports, browserModuleList)
