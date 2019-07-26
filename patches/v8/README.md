### Exporting node's patches to v8

```cmd
$ cd third_party/electron_node
$ CURRENT_NODE_VERSION=vX.Y.Z  # e.g. v10.11.0

# Find the last commit with the message "deps: update V8 to <some version>"
# This commit corresponds to node resetting V8 to its pristine upstream
# state at the stated version.
$ LAST_V8_UPDATE="$(git log --grep='^deps: update V8' --format='%H' -1 deps/v8)"

# This creates a patch file containing all changes in deps/v8 from
# $LAST_V8_UPDATE up to the current node version, formatted in a way that
# it will apply cleanly to the V8 repository (i.e. with `deps/v8`
# stripped off the path and excluding the v8/gypfiles directory, which
# isn't present in V8.
$ git format-patch \
    --relative=deps/v8 \
    $LAST_V8_UPDATE..$CURRENT_NODE_VERSION \
    deps/v8 \
    ':(exclude)deps/v8/gypfiles' \
    --stdout
```

When upgrading to a new version of node, make sure to match node's patches to
v8 by removing all the `deps_*` patches and re-exporting node's v8 patches
using the process above.
