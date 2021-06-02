self.addEventListener('fetch', async function (event) {
  const requestUrl = new URL(event.request.url);
  let responseTxt;
  if (requestUrl.pathname === '/echo' &&
    event.request.headers.has('X-Mock-Response')) {
    if (requestUrl.search === '?setBadge') {
      if (navigator.setAppBadge()) {
        try {
          await navigator.setAppBadge(42);
          responseTxt = 'SUCCESS setting app badge';
          await navigator.clearAppBadge();
        } catch (ex) {
          responseTxt = 'ERROR setting app badge ' + ex;
        }
      } else {
        responseTxt = 'ERROR navigator.setAppBadge is not available in ServiceWorker!';
      }
    } else if (requestUrl.search === '?clearBadge') {
      if (navigator.clearAppBadge()) {
        try {
          await navigator.clearAppBadge();
          responseTxt = 'SUCCESS clearing app badge';
        } catch (ex) {
          responseTxt = 'ERROR clearing app badge ' + ex;
        }
      } else {
        responseTxt = 'ERROR navigator.clearAppBadge is not available in ServiceWorker!';
      }
    }
    const mockResponse = new Response(responseTxt);
    event.respondWith(mockResponse);
  }
});
