self.addEventListener('fetch', function (event) {
  const requestUrl = new URL(event.request.url);

  if (requestUrl.pathname === '/echo' &&
    event.request.headers.has('X-Mock-Response')) {
    const mockResponse = new Response('Hello from serviceWorker!');
    event.respondWith(mockResponse);
  }
});
