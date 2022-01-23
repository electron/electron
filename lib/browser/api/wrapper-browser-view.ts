import { BaseView } from 'electron/main';
import type { WrapperBrowserView as WBVT } from 'electron/main';
const { WrapperBrowserView } = process._linkedBinding('electron_browser_wrapper_browser_view') as { WrapperBrowserView: typeof WBVT };

Object.setPrototypeOf(WrapperBrowserView.prototype, BaseView.prototype);

module.exports = WrapperBrowserView;
