import { EventEmitter } from 'events';
import type { BaseView as TLVT } from 'electron/main';
const { BaseView } = process._linkedBinding('electron_browser_base_view') as { BaseView: typeof TLVT };

Object.setPrototypeOf(BaseView.prototype, EventEmitter.prototype);

export default BaseView;
