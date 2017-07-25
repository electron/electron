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
Niebezpieczeństwo związane z wykonywaniem kodu występuje wszędzie gdzie ten kod otrzymujesz, a wykonywany jest lokalnie. Na przykład wyobraź sobie osadzoną stronę w oknie przeglądarki. Jeżeli atakujący w jakiś sposób zmieni treść (atakując źródło lub podmieniając przesyłane treści), będzie w stanie wykonać kod natywnie na maszynie użytkownika. 

> :warning: 
W żadnym wypadku nie należy ładować i wykonywać zdalnego kodu z włączoną integracją Node. Zamiast tego należy używać tylko plików lokalnych (pakowanych razem z aplikacją) w celu wykonania kodu Node. Aby wyświetlić zawartość zdalną, użyj tagu `webview` i upewnij się, że wyłączono `nodeIntegration`.

#### Checklist
Nie jest to lista wyczerpująca temat bezpieczeństwa, ale może pomóc podnieść jej poziom:

* Wyświetlaj tylko bezpieczną zawartość (https)
* Wyłącz integrację Node we wszystkich rendererach, które wyświetlają zdalną zawartość
* Włącz izolowanie kontekstu we wszystkich rendererach, które wyświetlają zdalną zawartość
* Użyj `ses.setPermissionRequestHandler()` we wszystkich sesjach, które ładują zdalną zawartość
* Nie wyłączaj `webSecurity`. Wyłączenie tego, wyłączy regułę same-origin
* Zdefiniuj regułę [`Content-Security`](http://www.html5rocks.com/en/tutorials/security/content-security-policy/)
* Nadpisz i wyłącz [`eval`](https://github.com/nylas/N1/blob/0abc5d5defcdb057120d726b271933425b75b415/static/index.js#L6-L8), który pozwala na uruchamianie string jako kodu
* Nie ustawiaj `allowRunningInsecureContent` jako true
* Nie włączaj `experimentalFeatures` i `experimentalCanvasFeatures` jeżeli nie jesteś pewien, że wiesz co robisz
* Nie używaj `blinkFeatures` jeżeli nie jesteś pewien, że wiesz co robisz
* WebViews: Nie dodawaj atrybutu `nodeintegration`
* WebViews: Nie używaj `disablewebsecurity`
* WebViews: Nie używaj `allowpopups`
* WebViews: Nie używaj `insertCSS` i `executeJavaScript`ze zdalnym CSS/JS

Ponownie, lista jedynie minimalizuje zagrożenie, ale nie likwiduje go. Jeżeli chcesz wyświetlić stronę internetową przeglądarka jest bezpieczniejszą opcją.
