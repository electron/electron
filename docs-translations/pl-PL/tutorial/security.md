# Bezpieczeństwo, możliwości natywne i twoja odpowiedzialność
Jako web developerzy, zazwyczaj cieszymy się mocnym bezpieczeństwem naszych przeglądarek. Ryzyko związane z kodem, który piszemy jest relatywnei mała. Nasze strony posiadają ograniczoną zdolność i działają w `piaskownicach`, a także wierzymy, że użytkownicy korzystający z przeglądarki zbudowanej przez duże zespoły inżynierów są zdolni do szybkiego wykrywania nowych zagrożeń.  

Kiedy pracujesz z Electron, istotne jest aby zrozumieć, że nie jest on stroną internetową. Pozwala na wykorzystanie wbudowanych funkcjonalności aplikacji pulpitowych. Wykorzystujesz podobne technologie, ale twój kod jest znacznie potężniejszy. JavaScript ma dostęp do plików systemowych, powłoki i wielu innych funkcji. To pozwala tworzyć wysokiej jakości natywne aplikacje, jednak stanowi także zagrożenie ze względu na większe uprawnienia i możliwości, którymi dysponujesz. 

Pamiętając o tym, bądź ostrożny wyświetlając treści z niezabezpieczonych źródeł. Warto wspomnieć, że popularne aplikacje stworzone przy użyciu Electon (Atom, Slack, Visual Studio Code i inne) wyświetlają zazwyczaj zawartość lokalną (lub bezpieczną, zabezpieczoną zdalnie zawartość bez integracji Node). Jeżeli twoja aplikacja wykonuje kod ze źródeł dostępnych online, na twoich barkach spoczywa odpowiedzialność nad upewnieniem się, że kod nie jest niebezpieczny lub "złośliwy".

## Zgłaszanie błędów bezpieczeństwa
Po więcej informacji jak prawidłowo ujawnić luke w bezpieczeństwie Electon zajrzyj do [Bezpieczeństwo](https://github.com/electron/electron/tree/master/SECURITY.md)

## Chromium Security Issues and Upgrades
While Electron strives to support new versions of Chromium as soon as possible,
developers should be aware that upgrading is a serious undertaking - involving
hand-editing dozens or even hundreds of files. Given the resources and
contributions available today, Electron will often not be on the very latest
version of Chromium, lagging behind by either days or weeks.

We feel that our current system of updating the Chromium component strikes an
appropriate balance between the resources we have available and the needs of the
majority of applications built on top of the framework. We definitely are
interested in hearing more about specific use cases from the people that build
things on top of Electron. Pull requests and contributions supporting this
effort are always very welcome.

Podczas gdy Electron stara się wspierać nowe wersje Chromium najszybciej jak to możliwe, twórcy oprogramowania powinni być świadomi, że aktualizowanie jest poważnym przedsięwzięciem, podczas którego ręcznie edytowanych jest setki plików. Biorąc to pod uwagę wersja, nad którą pracują programiści może nie być jeszcze dostępna. Electron może nie działać na najnowszej wersji Chromium przez kilka dni lub tygodni.

## Ignorowanie powyższych informacji
> :warning: Under no circumstances should you load and execute remote code with
Node integration enabled. Instead, use only local files (packaged together with
your application) to execute Node code. To display remote content, use the
`webview` tag and make sure to disable the `nodeIntegration`.

Niebezpieczeństwo związane z wykonywaniem kodu występuje wszędzie gdzie ten kod otrzymujesz, a wykonywany jest lokalnie. Na przykład wyobraź sobie osadzoną stronę w oknie przeglądarki. Jeżeli atakujący w jakiś sposób zmieni treść (atakując źródło lub podmieniając przesyłane treści), będzie w stanie wykonać kod natywnie na maszynie użytkownika. 



#### Checklist

This is not bulletproof, but at the least, you should attempt the following:

* Only display secure (https) content
* Disable the Node integration in all renderers that display remote content
  (setting `nodeIntegration` to `false` in `webPreferences`)
* Enable context isolation in all renderers that display remote content
  (setting `contextIsolation` to `true` in `webPreferences`)
* Use `ses.setPermissionRequestHandler()` in all sessions that load remote content
* Do not disable `webSecurity`. Disabling it will disable the same-origin policy.
* Define a [`Content-Security-Policy`](http://www.html5rocks.com/en/tutorials/security/content-security-policy/)
, and use restrictive rules (i.e. `script-src 'self'`)
* [Override and disable `eval`](https://github.com/nylas/N1/blob/0abc5d5defcdb057120d726b271933425b75b415/static/index.js#L6-L8)
, which allows strings to be executed as code.
* Do not set `allowRunningInsecureContent` to true.
* Do not enable `experimentalFeatures` or `experimentalCanvasFeatures` unless
  you know what you're doing.
* Do not use `blinkFeatures` unless you know what you're doing.
* WebViews: Do not add the `nodeintegration` attribute.
* WebViews: Do not use `disablewebsecurity`
* WebViews: Do not use `allowpopups`
* WebViews: Do not use `insertCSS` or `executeJavaScript` with remote CSS/JS.

Again, this list merely minimizes the risk, it does not remove it. If your goal
is to display a website, a browser will be a more secure option.
