# Capabilities

A **capability** is a named feature the host provides. A script can require one:

```lumen
assertCapability('HTTP')
```

If the host does not provide it, the program stops with `Function error` and `assertCapability failed: capability HTTP is not present`. The desktop interpreter provides `FS`, `random` and `HTTP`. Embedded hosts and ports provide their own set, see [Platforms & Ports](../platforms/overview.md).

Put assertions at the top of a script so it fails fast on a host that cannot run it.
