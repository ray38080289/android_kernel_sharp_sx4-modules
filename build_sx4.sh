#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Build the AQUOS wish4 (sx4) vendor kernel modules against the certified GKI
# kernel the device runs (android15-6.6 2026-07_r11, ACK cc1e318bd6fc).
#
# Tree layout (same as the kleaf manifests):
#   $ROOT/kernel-6.6                     ACK common (fetched if missing)
#   $ROOT/kernel_device_modules-6.6      this repository
#   $ROOT/vendor/mediatek/kernel_modules connectivity + GPU modules
#   $ROOT/prebuilts/clang-r510928        the GKI compiler (fetched if missing)
# Output: $ROOT/out/dist/*.ko (debug info stripped)

set -euo pipefail

ACK_SHA=cc1e318bd6fc                 # 6.6.139-android15-8-gcc1e318bd6fc-ab16457230
GKI_BUILD=16457230                   # ci.android.com build of that kernel
CLANG=r510928

DM="$(cd "$(dirname "$0")" && pwd)"
ROOT="${ROOT:-$(dirname "$DM")}"
K="$ROOT/kernel-6.6"
V="$ROOT/vendor/mediatek/kernel_modules"
C="$ROOT/prebuilts/clang-$CLANG"
O="$ROOT/out"
JOBS="${JOBS:-$(nproc)}"

fetch() {
    if [ ! -f "$K/Makefile" ]; then
        mkdir -p "$K"
        curl -sSfL "https://android.googlesource.com/kernel/common/+archive/$ACK_SHA.tar.gz" | tar xz -C "$K"
    fi
    if [ ! -x "$C/bin/clang" ]; then
        mkdir -p "$C"
        curl -sSfL "https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+archive/refs/heads/main-kernel-build-2024/clang-$CLANG.tar.gz" | tar xz -C "$C"
    fi
    if [ ! -s "$O/gki.symvers" ]; then
        mkdir -p "$O"
        local page url
        page=$(curl -sSfL "https://ci.android.com/builds/submitted/$GKI_BUILD/kernel_aarch64/latest/kernel_aarch64_Module.symvers")
        url=$(grep -o 'https://storage.googleapis.com/android-build/builds/[^"]*kernel_aarch64_Module.symvers[^"]*' <<<"$page" | head -1 | sed 's/\\u0026/\&/g')
        curl -sSfL -o "$O/gki.symvers" "$url"
    fi
    [ -d "$V/connectivity" ] || { echo "missing $V" >&2; exit 1; }
}

export PATH="$C/bin:$PATH"
# O must be $ROOT/out: gen4m's wrapper finds the connectivity tree as $(O)/../vendor/...
MK=(make O="$O" ARCH=arm64 LLVM=1 LLVM_IAS=1 -j"$JOBS"
    KCONFIG_EXT_PREFIX="$DM/" DEVICE_MODULES_PATH="$DM"
    DEVICE_MODULES_REL_DIR="$(realpath --relative-to="$K" "$DM")"
    DEVCIE_MODULES_INCLUDE="-I$DM/include" KERNEL_SRC="$K")

configure() {
    mkdir -p "$O"
    "${MK[@]}" -C "$K" gki_defconfig >/dev/null
    local cfg="$DM/kernel/configs" overlays
    overlays=$(DEVICE_MODULES_DIR=. ROOT_DIR=. bash -c ". '$DM/build.config.sx4' >/dev/null 2>&1; echo \$DEFCONFIG_OVERLAYS")
    (cd "$O" && bash "$K/scripts/kconfig/merge_config.sh" -m .config \
        "$DM/arch/arm64/configs/mgk_64_k66_defconfig" \
        $(for f in $overlays; do echo "$cfg/$f"; done) "$cfg/sign.config" >/dev/null)
    "${MK[@]}" -C "$K" olddefconfig >/dev/null
    "${MK[@]}" -C "$K" modules_prepare >/dev/null
    cp "$O/gki.symvers" "$O/Module.symvers"
}

# ACK modules that are not part of GKI but ship in vendor_dlkm, in dependency order
INTREE="drivers/iio/buffer drivers/media/v4l2-core drivers/gpu/drm drivers/usb/phy
        net/wireless net/mac80211 drivers/perf drivers/power/reset"

build() {
    local syms="" d
    for d in $INTREE; do
        "${MK[@]}" -C "$K" M="$K/$d" KBUILD_EXTRA_SYMBOLS="$syms" modules
        syms="$syms $K/$d/Module.symvers"
    done
    "${MK[@]}" -C "$K" M="$DM" KBUILD_EXTRA_SYMBOLS="$syms" modules
    syms="$DM/Module.symvers $syms"
    # connectivity and GPU, in dependency order
    for d in connectivity/common connectivity/conninfra connectivity/wlan/adaptor/build/connac1x \
             connectivity/wlan/core/gen4m/build/connac1x/6833 connectivity/bt/mt66xx/wmt \
             connectivity/fmradio/Build/mt6631_6635 connectivity/gps/gps_stp gpu/mt6833; do
        local dir="$K" opts=()
        case $d in
            */gen4m/build/*) dir="$V/$d" ;;          # wrapper Makefile, runs make -C $K itself
            */bt/mt66xx/wmt) opts=(BT_PLATFORM=connac1x) ;;
            gpu/*) opts=(BUILD_RULE=OOT CONFIG_MALI_MEMORY_GROUP_MANAGER=y
                         CONFIG_MALI_PROTECTED_MEMORY_ALLOCATOR=y CONFIG_DMA_SHARED_BUFFER_TEST_EXPORTER=y
                         CONFIG_MALI_PLATFORM_NAME=mt6833 MTK_PLATFORM_VERSION=mt6833) ;;
        esac
        "${MK[@]}" -C "$dir" M="$V/$d" "${opts[@]}" \
            KBUILD_EXTRA_SYMBOLS="$syms" EXTRA_SYMBOLS="$syms" modules
        [ -f "$V/$d/Module.symvers" ] && syms="$syms $V/$d/Module.symvers"
    done
}

dist() {
    rm -rf "$O/dist"; mkdir -p "$O/dist"
    find "$DM" $(for d in $INTREE; do echo "$K/$d"; done) -name '*.ko' -exec cp -t "$O/dist" {} +
    find -L "$V" -name '*.ko' -exec cp -n -t "$O/dist" {} +
    "$C/bin/llvm-strip" --strip-debug "$O"/dist/*.ko
    echo "$(ls "$O/dist" | wc -l) modules in $O/dist"
}

fetch
configure
build
dist
