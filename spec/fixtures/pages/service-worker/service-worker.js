/* globals self, URL, Response */

self.addEventListener('fetch', function (event) {
  var requestUrl = new URL(event.request.url)

  if (requestUrl.pathname === '/echo' &&
    event.request.headers.has('X-Mock-Response')) {
    var mockResponse = new Response('Hello from serviceWorker!')
    event.respondWith(mockResponse)
  }
})
