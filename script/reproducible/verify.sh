#!/bin/bash -e
SCRIPT=`realpath $0`
buildir=/mnt

usage() { echo "Usage: $0 [-b $buildir] [-n] [CHECK_VERSION]" 1>&2; exit 1; }

setup_mac_for_dev() {
    export CHECK_VERSION=$1

    which -s brew
    if [[ $? != 0 ]] ; then
        # Install Homebrew
        ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    else
        brew update
    fi

    which -s python2
    if [[ $? != 0 ]] ; then
        # Install python2
        brew install python2
    fi

    which -s python3
    if [[ $? != 0 ]] ; then
        # Install python3
        brew install python3
    fi

    if [ ! -f /usr/local/bin/gtar ]; then
      brew install gnu-tar
      ln -fs /usr/local/bin/gtar /usr/local/bin/tar
    fi

    XCODE_VERSION=$(curl -v https://raw.githubusercontent.com/electron/electron/${CHECK_VERSION}/.circleci/config.yml | grep "xcode:"  | head -n1 | awk '{print $2}' | tr -d '"')
    echo "Since there is no way to automate XCode installation. Install XCode version $XCODE_VERSION manually."
    echo "Press Key to continue."
    open 'https://developer.apple.com/download/more/?q=xcode'
    read -n1 ans
}

nocheckdisk=false

while getopts ":b:n" o; do
    case "${o}" in
        b)
            buildir=${OPTARG}
            ;;
        n)
            nodiskcheck=true
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))
export CHECK_VERSION=$1

if [[ "$OSTYPE" == "linux-gnu" ]] && cat /proc/1/cgroup | grep docker &>/dev/null; then
  # If you are in docker run youself and that's it
  echo "Executing under docker image, continuing"
  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git $HOME/depot_tools
  export GCLIENT_PY3=0
  export PATH=$HOME/depot_tools:$PATH
elif [[ "$OSTYPE" == "linux-gnu" ]] && ! (cat /proc/1/cgroup | grep docker &>/dev/null); then
  # If you are in linux run yourself but inside the correct container
  IMAGE=$(curl -v https://raw.githubusercontent.com/electron/electron/${CHECK_VERSION}/.circleci/config.yml | grep "image:" | awk '{print $3}')
  docker run -ti -v ${buildir}:${buildir} ${EXTRA_DOCKER_ARGS} -v ${SCRIPT}:/verify.sh ${IMAGE} bash -e /verify.sh -b ${buildir} $([ "$nodiskcheck" = "true" ] && printf " -n") ${CHECK_VERSION}
  exit 0
elif [[ "$OSTYPE" == "darwin"* ]]; then
  # If you are on mac run youself pointing to the correct version of toolchain
  setup_mac_for_dev "${CHECK_VERSION}" || true
  echo "Executing under macOS image, continuing"
fi

export ELECTRON_OUT_DIR=Default
export ELECTRON_ENABLE_STACK_DUMPING=1

echo Check for disk space
export FREESPACE=$(BLOCKSIZE=1g df $buildir | tail -n1 | awk '{print $4}')
export MINSPACE=100 # number of gygabites needed to download and build electron

if [ $FREESPACE -ge $MINSPACE ] || [ "$nodiskcheck" = "true" ]; then
  echo yes enough free space
else
  echo not enough free space - ${MINSPACE}GB 
  exit 5
fi

export GN_CONFIG=release
export STRIP_BINARIES=true 
export GENERATE_SYMBOLS=true
export CHECK_DIST_MANIFEST=1

echo "Building $env:GN_CONFIG build"
git config --global core.longpaths true
mkdir -p $buildir
cd $buildir
export CHROMIUM_BUILDTOOLS_PATH=$buildir/src/buildtools
export SCCACHE_PATH=$buildir/src/electron/external_binaries/sccache
#export GCLIENT_EXTRA_ARGS="$GCLIENT_EXTRA_ARGS --custom-var=\"checkout_boto=True\" --custom-var=\"checkout_requests=True\""
gclient config --name "src/electron" --unmanaged $GCLIENT_EXTRA_ARGS "https://github.com/electron/electron"
gclient sync -f --with_branch_heads --with_tags
cd src
cd electron
git fetch
git checkout $CHECK_VERSION
gclient sync -f
cd ..
cd ..
export BUILD_CONFIG_PATH="//electron/build/args/release.gn"
# build out/Default
$SCCACHE_PATH --show-stats
cd src
# XXX: leave extra space after GN_EXTRA_ARG
# GN_EXTRA_ARG= 
gn gen out/Default --args="import(\"//electron/build/args/release.gn\") $GN_EXTRA_ARG cc_wrapper=\"$SCCACHE_PATH\""
ninja -C out/Default electron:electron_app
gn gen out/ffmpeg "--args=import(\"//electron/build/args/ffmpeg.gn\") $GN_EXTRA_ARGS"

if [ "$STRIP_BINARIES" == "true" ] && [ "`uname`" == "Linux" ]; then
  if [ x"$TARGET_ARCH" == x ]; then
    target_cpu=x64
  elif [ "$TARGET_ARCH" == "ia32" ]; then
    target_cpu=x86
  else
    target_cpu="$TARGET_ARCH"
  fi
  electron/script/copy-debug-symbols.py --target-cpu="$target_cpu" --out-dir=out/Default/debug -d out/Default --compress
  electron/script/strip-binaries.py --target-cpu="$target_cpu" -d out/Default
  electron/script/add-debug-link.py --target-cpu="$target_cpu" --debug-dir=out/Default/debug -d out/Default
fi

ninja -C out/ffmpeg electron:electron_ffmpeg_zip
ninja -C out/Default electron:electron_dist_zip
ninja -C out/Default electron:electron_mksnapshot_zip
ninja -C out/Default electron:hunspell_dictionaries_zip
ninja -C out/Default electron:electron_chromedriver_zip
ninja -C out/Default third_party/electron_node:headers
$SCCACHE_PATH --show-stats
$SCCACHE_PATH --stop-server

cd out
# loop needed since sometimes the output directory is locked before completing the compilation
while true; do
  sleep 1
  mv Default Default2 || continue
  mv ffmpeg ffmpeg2 || continue
  break
done
cd ..

gn gen out/Default --args="import(\"//electron/build/args/release.gn\") $GN_EXTRA_ARG cc_wrapper=\"$SCCACHE_PATH\""
ninja -C out/Default electron:electron_app
gn gen out/ffmpeg "--args=import(\"//electron/build/args/ffmpeg.gn\") $GN_EXTRA_ARGS"

if [ "$STRIP_BINARIES" == "true" ] && [ "`uname`" == "Linux" ]; then
  if [ x"$TARGET_ARCH" == x ]; then
    target_cpu=x64
  elif [ "$TARGET_ARCH" == "ia32" ]; then
    target_cpu=x86
  else
    target_cpu="$TARGET_ARCH"
  fi
  electron/script/copy-debug-symbols.py --target-cpu="$target_cpu" --out-dir=out/Default/debug -d out/Default --compress
  electron/script/strip-binaries.py --target-cpu="$target_cpu" -d out/Default
  electron/script/add-debug-link.py --target-cpu="$target_cpu" --debug-dir=out/Default/debug -d out/Default
fi

ninja -C out/ffmpeg electron:electron_ffmpeg_zip
ninja -C out/Default electron:electron_dist_zip
ninja -C out/Default electron:electron_mksnapshot_zip
ninja -C out/Default electron:hunspell_dictionaries_zip
ninja -C out/Default electron:electron_chromedriver_zip
ninja -C out/Default third_party/electron_node:headers
$SCCACHE_PATH --show-stats

# Check for local determinism, there is no reason in checking remote determinism if local is not satisfied
export PATH=$HOME/.local/bin:$PATH
pip install python-stripzip
for F in out/Default*/*.zip; do
  stripzip $F
done
python tools/determinism/compare_build_artifacts.py -f out/Default -s out/Default2

# Check for remote determinism comparing output zip hashes
curl https://github.com/electron/electron/releases/download/$CHECK_VERSION/SHASUMS256.txt > /tmp/SHASUMS256.txt

#echo Expected SHA256SUM
cat /tmp/SHASUMS256.txt

for F in out/Default*/*.zip; do
  sha256sum $F
done
