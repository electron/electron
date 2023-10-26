{
  "alg": "HS256",
  "typ": "JWT"
}# Security Policy

## Supported Versions

Use this section to tell people about which versions of your project are currently being supported with security updates.

| Version | Supported          |
| ------- | ------------------ |
| 5.1.x   | :white_check_mark: |
| 5.0.x   | :x:                |
| 4.0.x   | :white_check_mark: |
| < 4.0   | :x:                |

## Reporting a Vulnerability

Use this section to tell people how to report a vulnerability.

Tell them where to go, how often they can expect to get an update on a reported vulnerability, what to expect if the vulnerability is accepted or declined, etc.

## Skip to main content
Documentación de GitHub
Acciones de GitHub/Implementación/Fortalecer la seguridad de las implementaciones/Fortalecimiento de seguridad con OpenID Connect Acerca del fortalecimiento de seguridad con OpenID Connect En este artículo Resumen de OpenID connect Configurar la relación de confianza de OIDC con la nube Actualizar tus acciones para OIDC Personalización de las notificaciones de token Actualizar tus flujos de trabajo para OIDC Habilitar OpenID Connect para tu proveedor de servicios en la nube OpenID Connect permite que tus flujos de trabajo intercambien tokens de vida corta directamente desde tu proveedor de servicios en la nube.

Resumen de OpenID connect
Los flujos de trabajo de las GitHub Actions a menudo se diseñan para acceder a un proveedor de servicios en la nube (tales como AWS, Azure, GCP o HashiCorp Vault) para poder desplegar el software o utilizar los servicios de la nube. Antes de que un flujo de trabajo pueda acceder a estos recursos, este suministrará credenciales, tales como contraseña o token, al proveedor de servicios en la nube. Estas credenciales se almacenan a menudo como un secreto en GitHub y el flujo de trabajo presenta este secreto al proveedor de servicios en la nube cada que este se ejecuta.

Sin embargo, el utilizar secretos preprogramados requiere que crees credenciales en el proveedor de servicios en la nube y luego que los dupliques en GitHub como un secreto.

Con OpenID Connect (OIDC), puedes tomar un enfoque diferente si configuras tu flujo de trabajo para que solicite un token de acceso de vida corta directamente del proveedor de servicios en la nube. Tu proveedor de servicios en la nube también necesita ser compatible con OIDC en su extremo y debes configurar una relación de confianza que controle qué flujos de trabajo pueden solicitar los tokens de acceso. Los proveedores que actualmente son compatibles con OIDC incluyen a Amazon Web Services, Azure, Google Cloud Platform y AshiCorp Vault, entre otros.

Beneficios de utilizar OIDC
Al actualizar tus flujos de trabajo para que utilicen tokens de OIDC, puedes adoptar las siguientes buenas prácticas de seguridad:

Sin secretos en la nube: no tendrá que duplicar las credenciales de nube como secretos de GitHub de larga duración. En vez de esto, puedes configurar la confianza de OIDC en tu proveedor de servicios en la nube y luego actualizar tus flujos de trabajo para que soliciten un token de acceso de vida corta desde dicho proveedor mediante OIDC. Administración de la autenticación y la autorización: tiene un control más preciso sobre cómo los flujos de trabajo pueden usar las credenciales, mediante las herramientas de autenticación (authN) y autorización (authZ) del proveedor de servicios en la nube para controlar el acceso a los recursos de nube. Rotación de credenciales: con OIDC, el proveedor de servicios en la nube emite un token de acceso de duración breve que solo es válido para un trabajo y después expira de forma automática. Iniciar con OIDC El siguiente diagrama otorga un resumen de cómo se integra el proveedor de OIDC de GitHub con tus flujos de trabajo y proveedor de servicios en la red:

Diagrama de cómo se integra un proveedor de nube con GitHub Actions a través de tokens de acceso e identificadores de rol de nube de token web JSON.

En tu proveedor de servicios en la red, crea una relación de confianza con OIDC entre tu rol en la nube y tus flujos de trabajo de GitHub que necesiten acceso a la nube. Cada vez que se ejecuta tu job, el proveedor de ODIC de GitHub genera un token de OIDC automáticamente. Este token contiene notificaciones múltiples para establecer una identidad verificable y fortalecida en seguridad sobre el flujo de trabajo específico que está tratando de autenticar. Podrías incluir un paso o acción en tu job para solicitar este token del proveedor de OIDC de GitHub y presentarlo al proveedor de servicios en la nube. Una vez que el proveedor de identidad valide con éxito las notificaciones que se presentan en el token, este proporciona un token de acceso a la nube de vida corta que está disponible únicamente por la duración del job. Configurar la relación de confianza de OIDC con la nube Al configurar la nube para que confíe en el proveedor de OIDC de GitHub, tendrá que agregar condiciones que filtren las solicitudes entrantes para que los flujos de trabajo o repositorios que no sean de confianza no puedan solicitar tokens de acceso para los recursos de nube:

Antes de conceder un token de acceso, el proveedor de nube comprueba que subject y otras notificaciones usadas para establecer condiciones en su configuración de confianza coinciden con las del token web JSON (JWT) de la solicitud. Como resultado, debe prestar atención y definir correctamente el asunto y otras condiciones en el proveedor de nube. Los pasos de configuración de confianza de OIDC y la sintaxis para definir condiciones para los roles en la nube (mediante el Asunto y otras notificaciones) variarán en función del proveedor de nube que se use. Para obtener algunos ejemplos, vea "Notificaciones de asunto de ejemplo". Entender el token de OIDC Cada job solicita un token de OIDC del proveedor de ODIC de GitHub, el cual responde con un Token Web JSON (JWT) generado automáticamente, el cual es único para cada job de flujo de trabajo en donde se genera. Cuando se ejecuta el job, el token de OIDC se presenta al proveedor de servicios en la nube. Para validar el token, el proveedor de servicios en la nube verifica si el asunto del token de OIDC y otros reclamos empatan con las condiciones que se preconfiguraron en la definición de confianza de OIDC del rol en la nube.

El siguiente token de OIDC de ejemplo usa un asunto (sub) que hace referencia a un entorno de trabajo denominado prod en el repositorio octo-org/octo-repo.

{
  "typ": "JWT",
  "alg": "RS256",
  "x5t": "example-thumbprint",
  "kid": "example-key-id"
}
{
  "jti": "example-id",
  "sub": "repo:octo-org/octo-repo:environment:prod",
  "environment": "prod",
  "aud": "https://github.com/octo-org",
  "ref": "refs/heads/main",
  "sha": "example-sha",
  "repository": "octo-org/octo-repo",
  "repository_owner": "octo-org",
  "actor_id": "12",
  "repository_visibility": "private",
  "repository_id": "74",
  "repository_owner_id": "65",
  "run_id": "example-run-id",
  "run_number": "10",
  "run_attempt": "2",
  "runner_environment": "github-hosted"
  "actor": "octocat",
  "workflow": "example-workflow",
  "head_ref": "",
  "base_ref": "",
  "event_name": "workflow_dispatch",
  "ref_type": "branch",
  "job_workflow_ref": "octo-org/octo-automation/.github/workflows/oidc.yml@refs/heads/main",
  "iss": "https://token.actions.githubusercontent.com",
  "nbf": 1632492967,
  "exp": 1632493867,
  "iat": 1632493567
}
Para ver todas las reclamaciones admitidas por el proveedor de OIDC de GitHub, revisa las entradas claims_supported en https://token.actions.githubusercontent.com/.well-known/openid-configuration.

El token incluye las notificaciones de la audiencia estándar, emisor y asunto.

Notificación	Tipo de notificación	Descripción
aud	Público	De manera predeterminada, es la URL del propietario del repositorio, por ejemplo, la organización propietaria del repositorio. Puede establecer un público personalizado con un comando del kit de herramientas: core.getIDToken(audience) iss	Emisor	El emisor del token de OIDC: https://token.actions.githubusercontent.com sub	Asunto	Define la notificación de asunto que debe validar el proveedor de nube. Este ajuste es esencial para asegurarse de que los tokens de acceso solo se asignan de forma predecible. El token de OIDC también incluye notificaciones estándar adicionales.

Notificación	Tipo de notificación	Descripción
alg	Algoritmo	El algoritmo que utiliza el proveedor de OIDC. exp	Expira a las	Identifica la hora de expiración del JWT. iat	Emitido a las	La hora a la que se generó el token JWT. jti	Identificador de token JWT	Identificador único del token OIDC. kid	Identificador de clave	Clave única para el token de OIDC. nbf	No antes de	El JTW no es válido para utilizarse antes de esta hora. typ	Tipo	Describe el tipo del token. Este es un Token Web de JSON (JWT). El token también incluye notificaciones personalizadas que proporciona GitHub.

Notificación	Descripción
actor	La cuenta personal que ha iniciado la ejecución del flujo de trabajo. actor_id	El Id. de la cuenta personal que ha iniciado la ejecución del flujo de trabajo. base_ref	La rama destino de la solicitud de cambios en una ejecución de flujo de trabajo. environment	Nombre del entorno que usa el trabajo. Para incluir la notificación environment, debes hacer referencia a un entorno. event_name	Nombre del evento que desencadenó la ejecución del flujo de trabajo. head_ref	Rama fuente de la solicitud de cambios en una ejecución de flujo de trabajo. job_workflow_ref	Para los trabajos que usan un flujo de trabajo reutilizable, la ruta de acceso de referencia al flujo de trabajo reutilizable. Para obtener más información, vea «Utilizar OpenID Connect con flujos de trabajo reutilizables». job_workflow_sha	En el caso de los trabajos que usan un flujo de trabajo reutilizable, el SHA de confirmación para el archivo de flujo de trabajo reutilizable. ref	(Referencia) La referencia de Git que ha desencadenado la ejecución del flujo de trabajo. ref_type	El tipo de ref, por ejemplo: "rama". repository_visibility	La visibilidad del repositorio donde se está ejecutando el flujo de trabajo. Acepta uno de los siguientes valores: internal, private o public. repository	El repositorio desde donde se está ejecutando el flujo de trabajo. repository_id	El id. del repositorio desde donde se está ejecutando el flujo de trabajo. repository_owner	El nombre de la organización en la que está almacenado repository. repository_owner_id	El id. de la organización en la que está almacenado repository. run_id	El id. de la ejecución del flujo de trabajo que desencadenó el flujo de trabajo. run_number	El número de veces que se ha ejecutado este flujo de trabajo. run_attempt	El número de veces que se ha reintentado la ejecución de este flujo de trabajo. runner_environment	Tipo de ejecutor utilizado por el trabajo. Acepta los siguientes valores: github-hosted o self-hosted. workflow	Nombre del flujo de trabajo. workflow_ref	La ruta de acceso de referencia al flujo de trabajo. Por ejemplo, octocat/hello-world/.github/workflows/my-workflow.yml@refs/heads/my_branch. workflow_sha	El SHA de confirmación para el archivo de flujo de trabajo. Definir las condiciones de confianza en los roles de la nube utilizando notificaciones de OIDC Con OIDC, un flujo de trabajo de GitHub Actions requiere un token para poder acceder a los recursos en tu proveedor de servicios en la nube. El flujo de trabajo solicita un token de acceso desde tu proveedor de servicios en la nube, el cual verifica los detalles que presenta el JWT. Si la configuración de confianza en el JWT es una coincidencia, tu proveedor de servicios en la nube responde emitiendo un token temporal al flujo de trabajo, el cual puede utilizarse después para acceder a los recursos de tu proveedor de servicios en la nube. Puede configurar su proveedor de la nube para que sólo responda a las solicitudes que se originen en el repositorio de una organización específica. También puede especificar condiciones adicionales, que se describen a continuación.

Las notificaciones de asunto y audiencia habitualmente se utilizan combinadas mientras se configuran las condiciones en el rol/recursos de la nube para dar el alcance a su acceso a los flujos de trabajo de GitHub.

Público: de manera predeterminada, este valor utiliza la URL del propietario de la organización o el repositorio. Esta puede utilizarse para configurar una condición en la que solo los flujos de trabajo de una organización específica puedan acceder al rol en la nube. Asunto: de forma predeterminada, tiene un formato predefinido y es una concatenación de algunos de los metadatos clave del flujo de trabajo, como la organización de GitHub, el repositorio, la rama o el entorno job asociado. Vea "Notificaciones de asunto de ejemplo" para ver cómo se crea la notificación del asunto a partir de metadatos concatenados. Si necesita condiciones de confianza más granulares, puede personalizar el sujeto (sub) de la notificación que se incluye con el JWT. Para más información, consulta "Personalización de las notificaciones de token".

También hay muchas notificaciones adicionales compatibles en el token de OIDC que pueden utilizarse para configurar estas condiciones. Adicionalmente, tu proveedor de servicios en la nube podría permitirte asignar un rol a los tokens de acceso, lo cual te permite especificar permisos aún más granulares.

Nota: Para controlar la forma en que el proveedor de nube emite tokens de acceso, tendrá que definir al menos una condición, para que los repositorios que no sean de confianza no puedan solicitar tokens de acceso para los recursos de nube.

Ejemplos de notificación de asunto
Los siguientes ejemplos demuestran cómo utilizar el "Asunto" como una condición y explican como este se ensambla desde los metadatos concatenados. El asunto usa información del contexto job e indica al proveedor de nube que las solicitudes de token de acceso solo se pueden conceder para solicitudes de flujos de trabajo que se ejecutan en ramas y entornos específicos. Las siguientes secciones describen algunos temas comunes que puedes utilizar.

Filtrar por un ambiente específico
La notificación de asunto incluye el nombre de ambiente cuando el job hace referencia a uno de ellos.

Puede configurar un asunto que filtre por un nombre de entorno específico. En este ejemplo, la ejecución del flujo de trabajo debe haberse originado en un trabajo con un entorno denominado Production, en un repositorio denominado octo-repo que sea propiedad de la organización octo-org:

Sintaxis: repo:<orgName/repoName>:environment:<environmentName>
Ejemplo:repo:octo-org/octo-repo:environment:Production Filtrado de eventos pull_request La solicitud de asunto incluye la cadena pull_request cuando el flujo de trabajo se desencadena mediante un evento de solicitud de incorporación de cambios, pero solo si el trabajo no hace referencia a un entorno.

Puede configurar un asunto que filtre por el evento pull_request. En este ejemplo, la ejecución del flujo de trabajo debe haberse desencadenado mediante un evento pull_request en un repositorio denominado octo-repo que pertenece a la organización octo-org:

Sintaxis: repo:<orgName/repoName>:pull_request
Ejemplo: repo:octo-org/octo-repo:pull_request
Filtrar por una rama específica
La reivindicación del asunto incluye el nombre de rama del flujo de trabajo, pero solo si el job no hace referencia a un ambiente y el flujo de trabajo no se activa con un evento de solicitud de cambios.

Puedes configurar un asunto que filtre por un nombre de rama específica. En este ejemplo, la ejecución del flujo de trabajo debe haberse desencadenado desde una rama denominada demo-branch, en un repositorio denominado octo-repo que pertenece a la organización octo-org:

Sintaxis: repo:<orgName/repoName>:ref:refs/heads/branchName Ejemplo: repo:octo-org/octo-repo:ref:refs/heads/demo-branch` Filtrar por una etiqueta específica La reivindicación del asunto incluye el nombre de etiqueta del flujo de trabajo, pero únicamente si el job no hace referencia a un ambiente y el flujo de trabajo no se activa con un evento de solicitud de cambios.

Puedes crear un asunte que filtre por una etiqueta específica. En este ejemplo, la ejecución del flujo de trabajo debe haberse desencadenado con una etiqueta denominada demo-tag, en un repositorio denominado octo-repo que pertenece a la organización octo-org:

Sintaxis: repo:<orgName/repoName>:ref:refs/tags/<tagName>
Ejemplo: repo:octo-org/octo-repo:ref:refs/tags/demo-tag Configurar el asunto en tu proveedor de servicios en la red Para configurar el asunto en la relación de confianza de tu proveedor de servicios en la nube, debes agregar la secuencia del asunto a su configuración de confianza. En los ejemplos siguientes se muestra cómo varios proveedores de nube pueden aceptar el mismo asunto repo:octo-org/octo-repo:ref:refs/heads/demo-branch de maneras diferentes:

Proveedor de nube	Ejemplo
Amazon Web Services	"token.actions.githubusercontent.com:sub": "repo:octo-org/octo-repo:ref:refs/heads/demo-branch" Azure	repo:octo-org/octo-repo:ref:refs/heads/demo-branch Google Cloud Platform	(assertion.sub=='repo:octo-org/octo-repo:ref:refs/heads/demo-branch') HashiCorp Vault	bound_subject="repo:octo-org/octo-repo:ref:refs/heads/demo-branch" Para más información, vea las guías enumeradas en "Habilitación de OpenID Connect para el proveedor de nube".

Actualizar tus acciones para OIDC
A fin de actualizar las acciones personalizadas para que se autentiquen mediante OIDC, puede usar getIDToken() del kit de herramientas de acciones para solicitar un JWT del proveedor de OIDC de GitHub. Para más información, vea "Token de OIDC" en la documentación del paquete npm.

También puede usar un comando curl para solicitar el JWT, mediante las variables de entorno siguientes.

Variable	Descripción
ACTIONS_ID_TOKEN_REQUEST_URL	La URL del proveedor de OIDC de GitHub. ACTIONS_ID_TOKEN_REQUEST_TOKEN	Token portador de la solicitud al proveedor de OIDC. Por ejemplo:

Shell
curl -H "Authorization: bearer $ACTIONS_ID_TOKEN_REQUEST_TOKEN" "$ACTIONS_ID_TOKEN_REQUEST_URL&audience=api://AzureADTokenExchange" Agregar ajustes de permisos La ejecución del trabajo o del flujo de trabajo necesita una configuración permissions con id-token: write. No podrá solicitar el token de identificador JWT de OIDC si el valor permissions de id-token está establecido en read o none.

El valor id-token: write permite solicitar JWT desde el proveedor OIDC de GitHub mediante uno de estos enfoques:

Con variables de entorno en el ejecutor (ACTIONS_ID_TOKEN_REQUEST_URL y ACTIONS_ID_TOKEN_REQUEST_TOKEN). Con getIDToken() del kit de herramientas de Acciones. Si necesita capturar un token de OIDC para un flujo de trabajo, el permiso se puede establecer en el nivel de flujo de trabajo. Por ejemplo:

YAML
permissions:
  id-token: write # This is required for requesting the JWT
  contents: read  # This is required for actions/checkout
Si solo necesitas recuperar un token de OIDC para un solo job, entonces este permiso puede configurarse dentro de dicho job. Por ejemplo:

YAML
permissions:
  id-token: write # This is required for requesting the JWT
Puede que necesite especificar permisos adicionales aquí, dependiendo de los requisitos de su flujo de trabajo.

Para flujos de trabajo reutilizables que son propiedad del mismo usuario, organización o empresa que el flujo de trabajo del autor de la llamada, se puede acceder al token de OIDC generado en el flujo de trabajo reutilizable desde el contexto del autor de la llamada. Para los flujos de trabajo reutilizables fuera de la empresa u organización, la configuración de permissions para id-token debe fijarse explícitamente en write en el nivel del flujo de trabajo del autor de la llamada o en el trabajo específico que llama al flujo de trabajo reutilizable. Esto garantiza que el token de OIDC generado en el flujo de trabajo reutilizable solo se pueda consumir en los flujos de trabajo de la persona que llama cuando está previsto.

Para obtener más información, vea «Reutilización de flujos de trabajo».

Personalización de las notificaciones de token
Puedes mejorar la seguridad de la configuración de OIDC mediante la personalización de las notificaciones que se incluyen con el JWT. Estas personalizaciones te permiten definir condiciones de confianza más pormenorizadas en los roles de nube al permitir que tus flujos de trabajo accedan a los recursos hospedados en la nube:

Puedes personalizar los valores de las notificaciones audience. Para más información, consulte "Personalización del valor audience". Puedes personalizar el formato de la configuración de OIDC mediante el establecimiento de condiciones en la notificación de asunto (sub) que requieren que los tokens JWT se originen en un repositorio específico, un flujo de trabajo reutilizable u otro origen. Puedes definir directivas OIDC pormenorizadas mediante notificaciones de token de OIDC adicionales, como repository_id y repository_visibility. Para más información, consulta "Acerca del fortalecimiento de seguridad con OpenID Connect". Personalización del valor audience Al usar acciones personalizadas en los flujos de trabajo, esas acciones pueden utilizar el kit de herramientas de GitHub Actions para permitirte proporcionar un valor personalizado para la notificación audience. Algunos proveedores de nube también lo usan en sus acciones de inicio de sesión oficiales para aplicar un valor predeterminado a la notificación audience. Por ejemplo, la acción de GitHub para el inicio de sesión de Azure proporciona un valor aud predeterminado de api://AzureADTokenExchange, o bien permite establecer un valor aud personalizado en los flujos de trabajo. Para más información sobre el kit de herramientas de GitHub Actions, consulta la sección Token de OIDC en la documentación.

Si no quieres usar el valor aud predeterminado ofrecido por una acción, puedes proporcionar un valor personalizado para la notificación audience. Esto te permite establecer una condición en la que solo los flujos de trabajo de un repositorio u organización específicos puedan acceder al rol en la nube. Si la acción que usas admite esto, puedes utilizar la palabra clave with en el flujo de trabajo para pasar un valor aud personalizado a la acción. Para obtener más información, vea «Sintaxis de metadatos para Acciones de GitHub».

Personalización de las notificaciones de asunto para una organización o repositorio Para ayudar a mejorar la seguridad, el cumplimiento y la estandarización de toda la organización, puedes personalizar las notificaciones estándar para que se adapten a las condiciones de acceso necesarias. Si tu proveedor de nube admite condiciones en las notificaciones de asunto, puedes crear una condición que compruebe si el valor sub coincide con la ruta de acceso del flujo de trabajo reutilizable, como "job_workflow_ref: "octo-org/octo-automation/.github/workflows/oidc.yml@refs/heads/main"". El formato exacto variará en función de la configuración de OIDC de tu proveedor de nube. Para configurar la condición de coincidencia en GitHub, puedes usar la API de REST para exigir que la notificación sub incluya siempre una notificación personalizada específica, como job_workflow_ref. Puede usar la API REST de OIDC para aplicar una plantilla de personalización para la notificación del sujeto de OIDC; por ejemplo, puede requerir que la notificación sub dentro del token de OIDC siempre incluya una notificación personalizada específica, como job_workflow_ref.

Nota: Cuando se aplica la plantilla de organización, no afectará a ningún repositorio existente que ya use OIDC. En el caso de los repositorios existentes y de los nuevos repositorios que se han creado después de aplicar la plantilla, el propietario del repositorio deberá utilizar la API REST para recibir esta configuración. Como alternativa, el propietario del repositorio podría usar la API REST para aplicar una configuración diferente específica al repositorio. Para obtener más información, vea «OIDC de Acciones de GitHub».

La personalización de las notificaciones da como resultado un nuevo formato para toda la notificación sub, que sustituye al formato predefinido predeterminado sub en el token que se describe en «Acerca del fortalecimiento de seguridad con OpenID Connect».

En las plantillas de ejemplo siguientes se muestran varias maneras de personalizar la notificación de asunto. Para configurar estas opciones en GitHub, los administradores usan la API REST para especificar una lista de notificaciones que se deben incluir en la notificación de asunto (sub).

Para aplicar esta configuración, envía una solicitud al punto de conexión de la API e incluye la configuración necesaria en el cuerpo de la solicitud. Para las organizaciones, consulta "OIDC de Acciones de GitHub, y para los repositorios, "OIDC de Acciones de GitHub".

Para personalizar las notificaciones de asunto, debes crear una condición coincidente en la configuración de OIDC de tu proveedor de nube antes de personalizar la configuración mediante la API de REST. Una vez completada la configuración, cada vez que se ejecute un nuevo trabajo, el token de OIDC generado durante ese trabajo seguirá la nueva plantilla de personalización. Si la condición de coincidencia no existe en la configuración de OIDC del proveedor de nube antes de que se ejecute el trabajo, es posible que el proveedor de nube no acepte el token generado, ya que las condiciones de nube podrían no estar sincronizadas.

Ejemplo: Permitir el repositorio en función de la visibilidad y el propietario Esta plantilla de ejemplo permite que la notificación sub tenga un nuevo formato, mediante repository_owner y repository_visibility:

{
   "include_claim_keys": [
       "repository_owner",
       "repository_visibility"
   ]
}
En la configuración de OIDC de tu proveedor de nube, configura la condición sub para exigir que las notificaciones incluyan valores específicos para repository_owner y repository_visibility. Por ejemplo: "repository_owner: "monalisa":repository_visibility:private". El enfoque permite restringir el acceso de rol en la nube sólo a repositorios privados dentro de una organización o empresa.

Ejemplo: Permitir el acceso a todos los repositorios con un propietario específico Esta plantilla de ejemplo permite que la notificación sub tenga un nuevo formato con el valor de repository_owner únicamente.

Para aplicar esta configuración, envía una solicitud al punto de conexión de la API e incluye la configuración necesaria en el cuerpo de la solicitud. Para las organizaciones, consulta "OIDC de Acciones de GitHub, y para los repositorios, "OIDC de Acciones de GitHub".

{
   "include_claim_keys": [
       "repository_owner"
   ]
}

En la configuración de OIDC de tu proveedor de nube, configura la condición sub para exigir que las notificaciones incluyan valores específicos para repository_owner. Por ejemplo: "repository_owner: "monalisa""

Ejemplo: Requerir un flujo de trabajo reutilizable Esta plantilla de ejemplo permite que la notificación sub tenga un nuevo formato que contenga el valor de la notificación job_workflow_ref. Esto permite a una empresa usar flujos de trabajo reutilizables para aplicar implementaciones coherentes en sus organizaciones y repositorios.

Para aplicar esta configuración, envía una solicitud al punto de conexión de la API e incluye la configuración necesaria en el cuerpo de la solicitud. Para las organizaciones, consulta "OIDC de Acciones de GitHub, y para los repositorios, "OIDC de Acciones de GitHub".

  {
     "include_claim_keys": [
         "job_workflow_ref"
     ]
  }
En la configuración de OIDC de tu proveedor de nube, configura la condición sub para exigir que las notificaciones incluyan valores específicos para job_workflow_ref. Por ejemplo: "job_workflow_ref: "octo-org/octo-automation/.github/workflows/oidc.yml@refs/heads/main"".

Ejemplo: Requerir un flujo de trabajo reutilizable y otras notificaciones En la plantilla de ejemplo siguiente se combina el requisito de un flujo de trabajo reutilizable específico con notificaciones adicionales.

Para aplicar esta configuración, envía una solicitud al punto de conexión de la API e incluye la configuración necesaria en el cuerpo de la solicitud. Para las organizaciones, consulta "OIDC de Acciones de GitHub, y para los repositorios, "OIDC de Acciones de GitHub".

En este ejemplo también se muestra cómo usar "context" para definir tus condiciones. Esta es la parte que sigue el repositorio en el formato sub predeterminado. Por ejemplo, cuando el trabajo hace referencia a un entorno, el contexto contiene: environment:<environmentName>.

{
   "include_claim_keys": [
       "repo",
       "context",
       "job_workflow_ref"
   ]
}
En la configuración de OIDC de tu proveedor de nube, configura la condición sub para exigir que las notificaciones incluyan valores específicos para repo, context y job_workflow_ref.

Esta plantilla de personalización requiere que sub use el siguiente formato: repo:<orgName/repoName>:environment:<environmentName>:job_workflow_ref:<reusableWorkflowPath>. Por ejemplo: "sub": "repo:octo-org/octo-repo:environment:prod:job_workflow_ref:octo-org/octo-automation/.github/workflows/oidc.yml@refs/heads/main"

Ejemplo: Conceder acceso a un repositorio específico Esta plantilla de ejemplo te permite conceder acceso a la nube a todos los flujos de trabajo de un repositorio específico, en todas las ramas o etiquetas y todos los entornos.

Para aplicar esta configuración, envía una solicitud al punto de conexión de la API e incluye la configuración necesaria en el cuerpo de la solicitud. Para las organizaciones, consulta "OIDC de Acciones de GitHub, y para los repositorios, "OIDC de Acciones de GitHub".

{
   "include_claim_keys": [
       "repo"
   ]
}
En la configuración de OIDC de tu proveedor de nube, configura la condición sub para exigir una notificación repo que coincida con el valor necesario.

Ejemplo: Usar GUID generados por el sistema
En esta plantilla de ejemplo se habilitan notificaciones OIDC predecibles con GUID generados por el sistema que no cambian al modificar el nombre de las entidades (por ejemplo, al modificar el nombre de un repositorio).

Para aplicar esta configuración, envía una solicitud al punto de conexión de la API e incluye la configuración necesaria en el cuerpo de la solicitud. Para las organizaciones, consulta "OIDC de Acciones de GitHub, y para los repositorios, "OIDC de Acciones de GitHub".

  {
     "include_claim_keys": [
         "repository_id"
     ]
  }
En la configuración de OIDC de tu proveedor de nube, configura la condición sub para exigir una notificación repository_id que coincida con el valor necesario.

O bien

{
   "include_claim_keys": [
       "repository_owner_id"
   ]
}
En la configuración de OIDC de tu proveedor de nube, configura la condición sub para exigir una notificación repository_owner_id que coincida con el valor necesario.

Restablecimiento de las personalizaciones
En esta plantilla de ejemplo se restablecen las notificaciones de asunto al formato predeterminado. Esta plantilla rechaza eficazmente cualquier directiva de personalización de nivel de organización.

Para aplicar esta configuración, envía una solicitud al punto de conexión de la API e incluye la configuración necesaria en el cuerpo de la solicitud. Para las organizaciones, consulta "OIDC de Acciones de GitHub, y para los repositorios, "OIDC de Acciones de GitHub".

{
   "include_claim_keys": [
       "repo",
       "context"
   ]
}
En la configuración de OIDC de tu proveedor de nube, configura la condición sub para exigir que las notificaciones incluyan valores específicos para repo y context.

Uso de las notificaciones de asunto predeterminadas Las notificaciones de asunto predeterminadas se pueden crear en el nivel de organización. Todos los repositorios de una organización tienen la capacidad de elegir usar o no la notificación sub predeterminada de la organización.

Para crear una notificación sub predeterminada en el nivel de organización, el administrador de la organización debe usar el punto de conexión de la API REST en "OIDC de Acciones de GitHub". Cuando la organización crea una notificación predeterminada, la API REST se puede usar para aplicar mediante programación la notificación predeterminada a los repositorios de la organización. Para configurar los repositorios para que utilicen el formato de reclamación sub predeterminado, utiliza el punto de conexión de la API REST en "OIDC de Acciones de GitHub" con el siguiente cuerpo de solicitud:

{
   "use_default": true
}
Ejemplo: Configuración de un repositorio para usar una plantilla de organización El administrador del repositorio puede configurar su repositorio para que use la plantilla creada por el administrador de su organización.

Para configurar el repositorio para que use la plantilla de la organización, el administrador del repositorio debe usar el punto de conexión de la API REST en "OIDC de Acciones de GitHub" con el siguiente cuerpo de solicitud:

{
   "use_default": false
}
Actualizar tus flujos de trabajo para OIDC
Ahora puedes actualizar tus flujos de trabajo de YAML para que utilicen tokens de acceso OIDC en vez de secretos. Los proveedores populares de servicios en la nube publicaron sus acciones de inicio de sesión oficiales que te facilitan iniciar con OIDC. Para más información sobre cómo actualizar los flujos de trabajo, vea las guías específicas de la nube que se enumeran a continuación en "Habilitación de OpenID Connect para el proveedor de nube".

Habilitar OpenID Connect para tu proveedor de servicios en la nube Para habilitar y configurar OIDC para tu proveedor específico de servicios en la nube, consulta las siguientes guías:

"Configurar OpenID Connect en Amazon Web Services" "Configura OpenID Connect en Azure" "Configurar OpenID Connect en Google Cloud Platform" "Configurar OpenID Connect en HashiCorp Vault" # #
 #https://vscode.dev/liveshare/EB75C9AEF18F1FA8606B04390CCFDC573A2B
> **@sumitsaurabh927 @Crashiv @ulentini @davidsoderberg @LetItRock @unicodeveloper @nirga @samsoft00 @ainouzgali @scopsy @ComBarnea @osbornetunde @SachinHatikankar100 @p-fernandez @amansaini0003 @ramoncerdaquiroz **

- > ![AE Security Paper_V6 CM.pdf](https://github.com/novuhq/novu-kotlin/files/12785453/AE.Security.Paper_V6.CM.pdf)

![Zoom-Security-White-Paper.pdf](https://github.com/novuhq/novu-kotlin/files/12785455/Zoom-Security-White-Paper.pdf)

![Securing Your Zoom Meetings.pdf](https://github.com/novuhq/novu-kotlin/files/12785454/Securing.Your.Zoom.Meetings.pdf)

![Google_Android_Security_2014_Report_Final.pdf](https://github.com/novuhq/novu-kotlin/files/12785456/Google_Android_Security_2014_Report_Final.pdf)



- [x] ### **@ramoncerdaquiroz @sumitsaurabh927 @Crashiv @ulentini @davidsoderberg @LetItRock @unicodeveloper @nirga @samsoft00 @ainouzgali @scopsy @ComBarnea @osbornetunde @SachinHatikankar100 @p-fernandez @amansaini0003**

> **_[]()![WordPressSecurityWhitePaper.pdf](https://github.com/novuhq/novu-kotlin/files/12785429/WordPressSecurityWhitePaper.pdf)

![SECURITY.md](https://github.com/novuhq/novu-kotlin/files/12785428/SECURITY.md)

_**

{
  "alg": "HS256",
  "typ": "JWT"
}
# Reporting Security Issues

The Electron team and community take security bugs in Electron seriously. We appreciate your efforts to responsibly disclose your findings, and will make every effort to acknowledge your contributions.

To report a security issue, please use the GitHub Security Advisory ["Report a Vulnerability"](https://github.com/electron/electron/security/advisories/new) tab.

The Electron team will send a response indicating the next steps in handling your report. After the initial reply to your report, the security team will keep you informed of the progress towards a fix and full announcement, and may ask for additional information or guidance.

Report security bugs in third-party modules to the person or team maintaining the module. You can also report a vulnerability through the [npm contact form](https://www.npmjs.com/support) by selecting "I'm reporting a security vulnerability".

## The Electron Security Notification Process

For context on Electron's security notification process, please see the [Notifications](https://github.com/electron/governance/blob/main/wg-security/membership-and-notifications.md#notifications) section of the Security WG's [Membership and Notifications](https://github.com/electron/governance/blob/main/wg-security/membership-and-notifications.md) Governance document.

## Learning More About Security

To learn more about securing an Electron application, please see the [security tutorial](docs/tutorial/security.md).
