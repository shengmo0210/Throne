#!/bin/bash
set -e

source script/env_deploy.sh
[ "$GOOS" == "windows" ] && [ "$GOARCH" == "amd64" ] && DEST=$DEPLOYMENT/windows64 || true
[ "$GOOS" == "windows" ] && [ "$GOARCH" == "386" ] && DEST=$DEPLOYMENT/windows32 || true
[ "$GOOS" == "windows" ] && [ "$GOARCH" == "arm64" ] && DEST=$DEPLOYMENT/windows-arm64 || true
[ "$GOOS" == "linux" ] && [ "$GOARCH" == "amd64" ] && DEST=$DEPLOYMENT/linux-amd64 || true
[ "$GOOS" == "linux" ] && [ "$GOARCH" == "arm64" ] && DEST=$DEPLOYMENT/linux-arm64 || true
[ "$GOOS" == "darwin" ] && [ "$GOARCH" == "amd64" ] && DEST=$DEPLOYMENT/macos-amd64 || true
[ "$GOOS" == "darwin" ] && [ "$GOARCH" == "arm64" ] && DEST=$DEPLOYMENT/macos-arm64 || true

if [[ "$GOOS" == "windowslegacy" ]]; then
  GOOS="windows"
  GOCMD="$PWD/go/bin/go"
  if [[ $GOARCH == 'amd64' ]]; then
    DEST=$DEPLOYMENT/windowslegacy64
  else
    DEST=$DEPLOYMENT/windows32
  fi
else
  GOCMD="go"
fi

if [ -z $DEST ]; then
  echo "Please set GOOS GOARCH"
  exit 1
fi
rm -rf $DEST
mkdir -p $DEST

if [[ "$GOOS" == "windows" ]]; then
  if [[ "$GOARCH" == "386" ]]; then
    curl -fLso $DEST/updater.exe "https://github.com/throneproj/updater/releases/latest/download/updater-windows32.exe"
  else
    curl -fLso $DEST/updater.exe "https://github.com/throneproj/updater/releases/latest/download/updater-windows64.exe"
  fi
fi
if [[ "$GOOS" == "linux" ]]; then
  if [[ "$GOARCH" == "arm64" ]]; then
    curl -fLso $DEST/updater "https://github.com/throneproj/updater/releases/latest/download/updater-linux-arm64"
  else
    curl -fLso $DEST/updater "https://github.com/throneproj/updater/releases/latest/download/updater-linux-amd64"
  fi
  chmod +x $DEST/updater
fi

export CGO_ENABLED=0

#### Go: core ####
pushd core/server
pushd gen
protoc -I . --go_out=. --protorpc_out=. libcore.proto
popd
VERSION_SINGBOX=$(go list -m -f '{{.Version}}' github.com/sagernet/sing-box)
$GOCMD build -v -o $DEST -trimpath -ldflags "-w -s -X 'github.com/sagernet/sing-box/constant.Version=${VERSION_SINGBOX}'" -tags "with_clash_api,with_gvisor,with_quic,with_wireguard,with_utls,with_dhcp,with_tailscale"
popd
