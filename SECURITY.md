# Reporting Security Issues

The Electron team and community take security bugs in Electron seriously. We appreciate your efforts to responsibly disclose your findings, and will make every effort to acknowledge your contributions.

To report a security issue, please use the GitHub Security Advisory ["Report a Vulnerability"](https://github.com/electron/electron/security/advisories/new) tab.

The Electron team will send a response indicating the next steps in handling your report. After the initial reply to your report, the security team will keep you informed of the progress towards a fix and full announcement, and may ask for additional information or guidance.

Report security bugs in third-party modules to the person or team maintaining the module. You can also report a vulnerability through the [npm contact form](https://www.npmjs.com/support) by selecting "I'm reporting a security vulnerability".

## The Electron Security Notification Process

For context on Electron's security notification process, please see the [Notifications](https://github.com/electron/governance/blob/main/wg-security/membership-and-notifications.md#notifications) section of the Security WG's [Membership and Notifications](https://github.com/electron/governance/blob/main/wg-security/membership-and-notifications.md) Governance document.

## Learning More About Security

To learn more about securing an Electron application, please see the [security tutorial](docs/tutorial/security.md).

Acerca de GitHub Advanced Security
En este artículo
Acerca de GitHub Advanced Security
Acerca de las características de Advanced Security
Acerca de los flujos de trabajo iniciales para Advanced Security
GitHub pone a disposición de los clientes medidas adicionales de seguridad mediante una licencia de Advanced Security. Estas características también se habilitan para los repositorios públicos en GitHub.com.

GitHub Advanced Security se encuentra disponible para las cuentas empresariales en GitHub Enterprise Cloud y GitHub Enterprise Server. Algunas características de la GitHub Advanced Security también están disponibles para los repositorios públicos en GitHub.com.. Para más información, consulta "Planes de GitHub."

Para obtener información sobre GitHub Advanced Security for Azure DevOps, consulta Configuración de GitHub Advanced Security for Azure DevOps en Microsoft Learn.

Acerca de GitHub Advanced Security
GitHub tiene muchas características que te ayudan a mejorar y mantener la calidad de tu código. Algunas de ellas se incluyen en todos los planes, como el gráfico de dependencias y las Dependabot alerts. Otras características de seguridad requieren una licencia de GitHub Advanced Security (GHAS) para ejecutarse en otros repositorios aparte de los públicos de GitHub.com.

Para comprobar una licencia de GitHub Advanced Security, debes usar GitHub Enterprise. Para información sobre cómo actualizar a GitHub Enterprise con GitHub Advanced Security, consulta "Planes de GitHub" y "Acerca de la facturación de GitHub Advanced Security".

Nota: Si quieres probar la versión preliminar de GitHub Advanced Security con Azure Repos, consulta GitHub Advanced Security y Azure DevOps en nuestro sitio de recursos. Para obtener documentación, consulta Configuración de GitHub Advanced Security for Azure DevOps en Microsoft Learn.

Acerca de las características de Advanced Security
Una licencia de GitHub Advanced Security proporciona las siguientes características adicionales:

Code scanning : busca posibles vulnerabilidades de seguridad y errores de codificación en el código. Para obtener más información, vea «Acerca del examen de código».

Secret scanning - Detecta secretos, por ejemplo, claves y tokens, que se hayan insertado en repositorios privados. Alertas de examen de secretos para usuarios y alertas de examen de secretos para asociados están disponibles y de forma gratuita para repositorios públicos en GitHub.com. Si la protección de inserción está habilitada, también detecta secretos cuando se insertan en el repositorio. Para más información, consulta "Acerca del examen de secretos" y "Protección contra el envío de cambios para repositorios y organizaciones".

Revisión de dependencias: muestre el impacto total de los cambios en las dependencias y consulte los detalles de las versiones vulnerables antes de combinar una solicitud de incorporación de cambios. Para obtener más información, vea «Acerca de la revisión de dependencias».

En la siguiente tabla se resume la disponibilidad de las características de GitHub Advanced Security para los repositorios públicos y privados.

Repositorio público	Repositorio privado
without Advanced Security	Repositorio privado
with Advanced Security
Análisis de código			
Análisis de secretos			
Revisión de dependencias			
Para obtener información sobre las características de Advanced Security que se encuentran en desarrollo, consulte "Plan de desarrollo público de GitHub". Para información general de todas las características de seguridad, consulta "Características de seguridad de GitHub".

Las características de la GitHub Advanced Security se encuentran habilitadas para todos los repositorios públicos de GitHub.com. Las organizaciones que utilizan GitHub Enterprise Cloud con la Advanced Security pueden habilitar estas características adicionalmente para repositorios privados e internos. Para obtener más información, consulta la documentación de GitHub Enterprise Cloud.

Acerca de los flujos de trabajo iniciales para Advanced Security
Nota: Los flujos de trabajo de inicio de Advanced Security se han consolidado en la categoría "Security" (Seguridad) de la pestaña Actions (Acciones) de un repositorio. Esta nueva configuración está actualmente en versión beta y está sujeta a cambios.

GitHub proporciona flujos de trabajo de inicio para características de seguridad como code scanning. Puedes utilizar estos flujos de trabajo sugeridos para construir tus flujos de trabajo del code scanning en vez de comenzar desde cero.
Para información sobre los flujos de trabajo de inicio, consulta "Establecimiento de la configuración avanzada del examen de código con CodeQL" y "Using starter workflows".
