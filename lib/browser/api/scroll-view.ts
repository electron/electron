import { BaseView } from 'electron/main';
import type { ScrollView as SVT } from 'electron/main';
const { ScrollView } = process._linkedBinding('electron_browser_scroll_view') as { ScrollView: typeof SVT };

Object.setPrototypeOf(ScrollView.prototype, BaseView.prototype);

const isScrollView = (view: any) => {
  return view && view.constructor.name === 'ScrollView';
};

ScrollView.fromId = (id: number) => {
  const view = BaseView.fromId(id);
  return isScrollView(view) ? view as any as SVT : null;
};

ScrollView.getAllViews = () => {
  return BaseView.getAllViews().filter(isScrollView) as any[] as SVT[];
};

module.exports = ScrollView;
