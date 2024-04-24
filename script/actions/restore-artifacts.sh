#!/bin/sh

set -eo pipefail

mv_if_exist() {
  if [ -f "generated_artifacts_${BUILD_TYPE}/$1" ] || [ -d "generated_artifacts_${BUILD_TYPE}/$1" ]; then
    echo Restoring $1 to $2
    mkdir -p $2
    mv generated_artifacts_${BUILD_TYPE}/$1 $2
  else
    echo Skipping $1 - It is not present on disk
  fi
}

mv_if_exist dist.zip src/out/Default
mv_if_exist node_headers.tar.gz src/out/Default/gen
mv_if_exist symbols.zip src/out/Default
mv_if_exist mksnapshot.zip src/out/Default
mv_if_exist chromedriver.zip src/out/Default
mv_if_exist ffmpeg.zip src/out/ffmpeg
mv_if_exist hunspell_dictionaries.zip src/out/Default
mv_if_exist cross-arch-snapshots src