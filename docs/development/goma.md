# Goma

> Goma is a distributed compiler service for open-source projects such as
> Chromium and Android.

Electron has a deployment of a custom Goma Backend that we make available to
all Electron Maintainers.  See the [Access](#access) section below for details
on authentication.

## Enabling Goma

Currently Electron Goma supports Windows, Linux, and macOS.  If you are
on a supported platform you can enable goma by importing the `goma.gn` config
file when using `gn`.

```bash
gn gen out/Testing --args="import(\"//electron/build/args/testing.gn\") import(\"//electron/build/args/goma.gn\")"
```

You must ensure that you do not have `cc_wrapper` configured, this means you
can't use `sccache` or similar technology.

Before you can use goma to build Electron you need to authenticate against
the Goma service.  You only need to do this once per-machine.

```bash
cd electron/external_binaries/goma
./goma_auth.py login
```

Once authenticated you need to make sure the goma daemon is running on your
machine.

```bash
cd electron/external_binaries/goma
./goma_ctl.py ensure_start
```

## Building with Goma

When you are using Goma you can run `ninja` with a substantially higher `j`
value than would normally be supported by your machine.

Please do not set a value higher than **300** on Windows or Linux and
**80** on macOS, we monitor the goma system and users found to be abusing
it with unreasonable concurrency will be de-activated.

```bash
ninja -C out/Testing electron -j 200
```

## Monitoring Goma

If you access [http://localhost:8088](http://localhost:8088) on your local
machine you can monitor compile jobs as they flow through the goma system.

## Access

For security and cost reasons access to Electron Goma is currently restricted
to Electron Maintainers.  If you want access please head to `#access-requests` in
Slack and ping `@goma-squad` to ask for access.  Please be aware that being a
maintainer does not *automatically* grant access and access is determined on a
case by case basis.
