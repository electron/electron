#!/bin/sh

set -eo pipefail

if [ -z "$MAS_BUILD" ]; then
  BUILD_TYPE="darwin"
else
  BUILD_TYPE="mas"
fi

rm -rf generated_artifacts_${BUILD_TYPE}
mkdir generated_artifacts_${BUILD_TYPE}

mv_if_exist() {
  if [ -f "$1" ] || [ -d "$1" ]; then
    echo Storing $1
    mv $1 generated_artifacts_${BUILD_TYPE}
  else
    echo Skipping $1 - It is not present on disk
  fi
}
cp_if_exist() {
  if [ -f "$1" ] || [ -d "$1" ]; then
    echo Storing $1
    cp $1 generated_artifacts_${BUILD_TYPE}
  else
    echo Skipping $1 - It is not present on disk
  fi
}

mv_if_exist src/out/Default/dist.zip
mv_if_exist src/out/Default/gen/node_headers.tar.gz
mv_if_exist src/out/Default/symbols.zip
mv_if_exist src/out/Default/mksnapshot.zip
mv_if_exist src/out/Default/chromedriver.zip
mv_if_exist src/out/ffmpeg/ffmpeg.zip
mv_if_exist src/out/Default/hunspell_dictionaries.zip
mv_if_exist src/cross-arch-snapshots
cp_if_exist src/out/electron_ninja_log
cp_if_exist src/out/Default/.ninja_log
