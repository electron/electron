window.delayed = true;

global.getGlobalNames = () => {
  return Object.getOwnPropertyNames(global)
    .filter(key => typeof global[key] === 'function')
    .filter(key => key !== 'WebView')
    .sort();
};

const atPreload = global.getGlobalNames();

window.addEventListener('load', () => {
  window.test = {
    atPreload,
    atLoad: global.getGlobalNames()
  };
});
