import { View } from 'electron';

const { WebContentsView } = process.electronBinding('web_contents_view', 'browser');

Object.setPrototypeOf(WebContentsView.prototype, View.prototype);

export default WebContentsView;
