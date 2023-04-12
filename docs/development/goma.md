# Goma

> Goma is a distributed compiler service for open-source projects such as
> Chromium and Android.

Electron has a deployment of a custom Goma Backend that we make available to
all Electron Maintainers.  See the [Access](#access) section below for details
on authentication.  There is also a `cache-only` Goma endpoint that will be
used by default if you do not have credentials.  Requests to the cache-only
Goma will not hit our cluster, but will read from our cache and should result
in significantly faster build times.

## Enabling Goma

Currently the only supported way to use Goma is to use our [Build Tools](https://github.com/electron/build-tools).
Goma configuration is automatically included when you set up `build-tools`.

If you are a maintainer and have access to our cluster, please ensure that you run
`e init` with `--goma=cluster` in order to configure `build-tools` to use
the Goma cluster.  If you have an existing config, you can just set `"goma": "cluster"`
in your config file.

## Building with Goma

When you are using Goma you can run `ninja` with a substantially higher `j`
value than would normally be supported by your machine.

Please do not set a value higher than **200**. We monitor Goma system usage, and users
found to be abusing it with unreasonable concurrency will be de-activated.

```bash
ninja -C out/Testing electron -j 200
```

If you're using `build-tools`, appropriate `-j` values will automatically be used for you.

## Monitoring Goma

If you access [http://localhost:8088](http://localhost:8088) on your local
machine you can monitor compile jobs as they flow through the goma system.

## Access

For security and cost reasons, access to Electron's Goma cluster is currently restricted
to Electron Maintainers.  If you want access please head to `#access-requests` in
Slack and ping `@goma-squad` to ask for access.  Please be aware that being a
maintainer does not _automatically_ grant access and access is determined on a
case by case basis.

## Uptime / Support

We have automated monitoring of our Goma cluster and cache at https://status.notgoma.com

We do not provide support for usage of Goma and any issues raised asking for help / having
issues will _probably_ be closed without much reason, we do not have the capacity to handle
that kind of support.
