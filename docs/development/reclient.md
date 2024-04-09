# Reclient

> Reclient integrates with an existing build system to enable remote execution and caching of build actions.

Electron has a deployment of a [reclient](https://github.com/bazelbuild/reclient)
compatible RBE Backend that is available to all Electron Maintainers.
See the [Access](#access) section below for details on authentication. Non-maintainers
will not have access to the cluster, but can sign in to receive a `Cache Only` token
that gives access to the cache-only CAS backend. Using this should result in
significantly faster build times .

## Enabling Reclient

Currently the only supported way to use Reclient is to use our [Build Tools](https://github.com/electron/build-tools).
Reclient configuration is automatically included when you set up `build-tools`.

If you have an existing config, you can just set `"reclient": "remote_exec"`
in your config file.

## Building with Reclient

When you are using Reclient, you can run `autoninja` with a substantially higher `j`
value than would normally be supported by your machine.

Please do not set a value higher than **200**. The RBE system is monitored.
Users found to be abusing it with unreasonable concurrency will be deactivated.

```bash
autoninja -C out/Testing electron -j 200
```

If you're using `build-tools`, appropriate `-j` values will automatically be used for you.

## Access

For security and cost reasons, access to Electron's RBE backend is currently restricted
to Electron Maintainers.  If you want access, please head to `#access-requests` in
Slack and ping `@infra-wg` to ask for it.  Please be aware that being a
maintainer does not _automatically_ grant access. Access is determined on a
case-by-case basis.

## Support

We do not provide support for usage of Reclient. Issues raised asking for help / having
issues will _probably_ be closed without much reason. We do not have the capacity to handle
that kind of support.
