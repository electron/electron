import { View } from 'electron/main';

const { WebContentsView } = process._linkedBinding('electron_browser_web_contents_view');

Object.setPrototypeOf(WebContentsView.prototype, View.prototype);

export default WebContentsView;
