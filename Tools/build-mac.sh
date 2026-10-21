#!/bin/bash
set -euo pipefail
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="$PROJECT_ROOT/MaterialHall.uproject"
UE_ROOT="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
ACTION="${1:-Package}"
if [[ -z "${DEVELOPER_DIR:-}" ]]; then
    if [[ -d /Applications/Xcode.app/Contents/Developer ]]; then
        export DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer
    elif [[ -d "$HOME/Downloads/Xcode.app/Contents/Developer" ]]; then
        export DEVELOPER_DIR="$HOME/Downloads/Xcode.app/Contents/Developer"
    fi
fi
BUILD="$UE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh"
if [[ ! -f "$BUILD" ]]; then
    echo "Unreal Engine 5.8.4 not found. Set UE_ROOT to its install directory." >&2
    exit 1
fi
case "$ACTION" in Editor|Validate|Package) ;; *) echo 'Usage: build-mac.sh [Editor|Validate|Package]' >&2; exit 2;; esac
"$BUILD" MaterialHallEditor Mac Development "-Project=$PROJECT" -architecture=arm64 -NoHotReload -NoUBA
if [[ "$ACTION" == Editor ]]; then exit 0; fi
"$UE_ROOT/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PROJECT" \
    -run=pythonscript "-script=$PROJECT_ROOT/Tools/validate_scene.py" -nullrhi -unattended -nosplash
if [[ "$ACTION" == Validate ]]; then exit 0; fi
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.command" BuildCookRun \
    "-project=$PROJECT" -noP4 -platform=Mac -architecture=arm64 -clientconfig=Development \
    -build -cook -stage -pak -iostore -map=/Game/MaterialHall/WhiteHall \
    -unattended -utf8output -NoCompileEditor -UbtArgs=-NoUBA
APP="$PROJECT_ROOT/Saved/StagedBuilds/Mac/MaterialHall.app"
codesign --verify --deep --strict "$APP"
echo "Complete standalone bundle: $APP"
