---
title: 'Adding Features'
description: 'In this step of the tutorial, we will share some resources you should read to add features to your application'
slug: tutorial-adding-features
hide_title: false
---

:::info Follow along the tutorial

This is **part 4** of the Electron tutorial.

1. [Prerequisites][prerequisites]
1. [Building your First App][building your first app]
1. [Using Preload Scripts][preload]
1. **[Adding Features][features]**
1. [Packaging Your Application][packaging]
1. [Publishing and Updating][updates]

:::

## Añadiendo complejidad a la aplicación

Si has seguido las instrucciones, deberías tener una aplicación funcional de Electron con una interfaz de usuario estática. Desde este punto de partida, generalmente puedes avanzar en el desarrollo de tu aplicación en dos direcciones amplias:

1. Añadiendo complejidad al código de la aplicación web en el proceso del renderizador.
2. Integraciones más profundas con el sistema operativo y Node.js.

Es importante entender la distinción entre estos dos conceptos amplios. Para el primer punto, no son necesarios recursos específicos de Electron. Construir una lista de tareas atractiva en Electron es simplemente apuntar tu BrowserWindow de Electron a una aplicación web de lista de tareas atractiva. En última instancia, estás construyendo la interfaz de usuario de tu renderizador usando las mismas herramientas (HTML, CSS, JavaScript) que usarías en la web. Por lo tanto, la documentación de Electron no profundizará en cómo usar las herramientas web estándar.

Por otro lado, Electron también proporciona un conjunto rico de herramientas que te permiten integrarte con el entorno de escritorio, desde la creación de iconos de bandeja hasta la adición de atajos globales y la visualización de menús nativos. También te da todo el poder de un entorno Node.js en el proceso principal. Este conjunto de capacidades separa las aplicaciones de Electron de ejecutar un sitio web en una pestaña del navegador, y son el enfoque de la documentación de Electron.

## Ejemplos prácticos

La documentación de Electron tiene muchos tutoriales para ayudarte con temas más avanzados e integraciones más profundas con el sistema operativo. Para comenzar, revisa el documento [Ejemplos Prácticos][how-to].


:::nota ¡Déjanos saber si falta algo!

Si no encuentras lo que estás buscando, por favor avísanos en [GitHub][] o en nuestro [servidor de Discord][discord].

:::

## ¿Qué sigue?

Para el resto del tutorial, nos alejaremos del código de la aplicación y te mostraremos cómo puedes llevar tu aplicación desde tu máquina de desarrollo a las manos de los usuarios finales.

<!-- Link labels -->

[discord]: https://discord.gg/electronjs
[github]: https://github.com/electron/website/issues/new
[how-to]: ./examples.md

<!-- Tutorial links -->

[prerequisites]: tutorial-1-prerequisites.md
[building your first app]: tutorial-2-first-app.md
[preload]: tutorial-3-preload.md
[features]: tutorial-4-adding-features.md
[packaging]: tutorial-5-packaging.md
[updates]: tutorial-6-publishing-updating.md
