## get-patch

Use it to save commits from upstream repositories as patch files.

### Examples

1. Write commit contents in the patch format to stdout.
```
$ ./script/get-patch --repo src --commit 8e8216e5
```

2. Create a patch file with a default name if a specified directory.
```
$ ./script/get-patch --repo src --commit 8e8216e5 --output-dir patches
```

3. Create a patch file with a custom name in the current directory.
```
$ ./script/get-patch --repo src --commit 8e8216e5 --filename my.patch
```

4. Create a patch file with a custom name in a specified directory.
```
$ ./script/get-patch --repo src --commit 8e8216e5 --output-dir patches --filename my.patch
```

5. Create patch files with default names in a specified directory.
```
$ ./script/get-patch --repo src --output-dir patches --commit 8e8216e5 164c37e3
```

6. Create patch files with custom names in a specified directory.
Note that number of filenames must match the number of commits.
```
$ ./script/get-patch --repo src --output-dir patches --commit 8e8216e5 164c37e3 --filename first.patch second.patch
```
