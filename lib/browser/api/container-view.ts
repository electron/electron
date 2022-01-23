import { BaseView } from 'electron/main';
import type { ContainerView as CVT } from 'electron/main';
const { ContainerView } = process._linkedBinding('electron_browser_container_view') as { ContainerView: typeof CVT };

Object.setPrototypeOf(ContainerView.prototype, BaseView.prototype);

const isContainerView = (view: any) => {
  return view && view.constructor.name === 'ContainerView';
};

ContainerView.fromId = (id: number) => {
  const view = BaseView.fromId(id);
  return isContainerView(view) ? view as any as CVT : null;
};

ContainerView.getAllViews = () => {
  return BaseView.getAllViews().filter(isContainerView) as any[] as CVT[];
};

module.exports = ContainerView;
