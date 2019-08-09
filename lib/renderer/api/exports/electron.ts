import { defineProperties } from '@electron/internal/common/define-properties'
import { commonModuleList } from '@electron/internal/common/api/module-list'
import { rendererModuleList } from '@electron/internal/renderer/api/module-list'

defineProperties(exports, commonModuleList)
defineProperties(exports, rendererModuleList)
