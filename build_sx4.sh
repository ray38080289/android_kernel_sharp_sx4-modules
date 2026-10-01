#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Build the AQUOS wish4 (sx4) vendor kernel modules against the certified GKI
# kernel the device runs (android15-6.6 2026-07_r11, ACK cc1e318bd6fc).
#
# Tree layout (same as the kleaf manifests):
#   $ROOT/kernel-6.6                     ACK common (fetched if missing)
#   $ROOT/kernel_device_modules-6.6      this repository
#   $ROOT/vendor/mediatek/kernel_modules connectivity + GPU modules (fetched if missing:
#                                        LineageOS lamu-mediatek_modules + patches/mediatek_kernel_modules)
#   $ROOT/prebuilts/clang-r510928        the GKI compiler (fetched if missing)
# Output: $ROOT/out/dist/*.ko (debug info stripped), mt6833.dtb and dtbo.img
#
# ./build_sx4.sh dtbs builds only the device tree images.
# ./build_sx4.sh lineage DIR builds everything and assembles DIR as device/sharp/sx4-kernels/6.6
# for the LineageOS tree: the certified GKI Image.gz and system_dlkm modules from ci.android.com,
# the vendor modules and device tree images built here, and the module load lists in lineage/.

set -euo pipefail

ACK_SHA=cc1e318bd6fc                 # 6.6.139-android15-8-gcc1e318bd6fc-ab16457230
GKI_BUILD=16457230                   # ci.android.com build of that kernel
CLANG=r510928
MTK_MODULES=635ca317b538541c6c33c1abee9aa56b01aeb407  # LineageOS android_kernel_motorola_lamu-mediatek_modules
LIBUFDT=df92216e3fc5f3fe289423a7819bd4a5ed3bd443  # mkdtboimg.py (LineageOS android_system_libufdt)

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
    if [ ! -s "$O/mkdtboimg.py" ]; then
        mkdir -p "$O"
        curl -sSfL -o "$O/mkdtboimg.py"             "https://raw.githubusercontent.com/LineageOS/android_system_libufdt/$LIBUFDT/utils/src/mkdtboimg.py"
    fi
    if [ ! -d "$V/connectivity" ]; then
        git init -q "$V"
        git -C "$V" fetch -q --depth 1 https://github.com/LineageOS/android_kernel_motorola_lamu-mediatek_modules "$MTK_MODULES"
        git -C "$V" checkout -q FETCH_HEAD
        git -C "$V" apply "$DM"/patches/mediatek_kernel_modules/*.patch
    fi
}

export PATH="$C/bin:$PATH"
# Device and vendor modules are built with M= relative to the kernel tree, as MediaTek's
# build does: several of their Makefiles build include paths as $(srctree)/$(src).
# O must be $ROOT/out: gen4m's wrapper finds the connectivity tree as $(O)/../vendor/...
REL="$(realpath --relative-to="$K" "$DM")"
VREL="$(realpath --relative-to="$K" "$V")"
MK=(make O="$O" ARCH=arm64 LLVM=1 LLVM_IAS=1 -j"$JOBS"
    KCONFIG_EXT_PREFIX="$REL/" DEVICE_MODULES_REL_DIR="$REL"
    DEVICE_MODULES_PATH="\$(srctree)/$REL" DEVCIE_MODULES_INCLUDE="-I\$(srctree)/$REL/include"
    KERNEL_SRC="$K")

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
    "${MK[@]}" -C "$K" M="$REL" KBUILD_EXTRA_SYMBOLS="$syms" modules
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
        "${MK[@]}" -C "$dir" M="$VREL/$d" "${opts[@]}" \
            KBUILD_EXTRA_SYMBOLS="$syms" EXTRA_SYMBOLS="$syms" modules
        [ -f "$V/$d/Module.symvers" ] && syms="$syms $V/$d/Module.symvers"
    done
}

dist() {
    rm -rf "$O/dist"; mkdir -p "$O/dist"
    find "$DM" $(for d in $INTREE; do echo "$K/$d"; done) -name '*.ko' -exec cp -t "$O/dist" {} +
    find -L "$V" -name '*.ko' -exec cp -n -t "$O/dist" {} +
    "$C/bin/llvm-strip" --strip-debug "$O"/dist/*.ko
    echo "$(ls "$O"/dist/*.ko | wc -l) modules in $O/dist"
}

# Base dtb and the six board overlays (EVB, PRE_EVT, EVT, DVT, PVT, MP = ids 0-5; LK
# picks the id), each packed in a DT table image like the stock dtb and dtbo partitions.
dtbs() {
    local d="$DM/arch/arm64/boot/dts" b i=0 ovl=()
    mkdir -p "$O/dts" "$O/dist"
    dtc_one() {
        clang -E -nostdinc -I"$DM/include" -I"$K/include" -I"$d" -I"$d/mediatek"             -I"$K/scripts/dtc/include-prefixes" -undef -D__DTS__ -x assembler-with-cpp             -o "$O/dts/$2.pre" "$d/mediatek/$1"
        "$O/scripts/dtc/dtc" -@ -q -I dts -O dtb -o "$O/dts/$2" "$O/dts/$2.pre"
    }
    dtc_one mt6833.dts mt6833.dtb
    for b in evb pre_evt evt dvt pvt mp; do
        dtc_one "k6833v1_64_sx4_$b.dts" "sx4_$b.dtbo"
        ovl+=("$O/dts/sx4_$b.dtbo" --id=$i); i=$((i + 1))
    done
    python3 "$O/mkdtboimg.py" create "$O/dist/mt6833.dtb" --page_size=2048 "$O/dts/mt6833.dtb" --id=0
    python3 "$O/mkdtboimg.py" create "$O/dist/dtbo.img" --page_size=2048 "${ovl[@]}"
    # drop the page padding after the table, as in the stock images
    for b in mt6833.dtb dtbo.img; do
        truncate -s "$(od -An -tu4 --endian=big -j4 -N4 "$O/dist/$b")" "$O/dist/$b"
    done
    echo "device tree: $O/dist/mt6833.dtb $O/dist/dtbo.img"
}

# ci.android.com artifact of the GKI build: signed URL from the artifact page
ci_get() {
    local page url
    page=$(curl -sSfL "https://ci.android.com/builds/submitted/$GKI_BUILD/kernel_aarch64/latest/$1")
    url=$(grep -o 'https://storage.googleapis.com[^"]*' <<<"$page" | head -1 | sed -E 's/\\+u0026/\&/g')
    curl -sSfL -o "$2" "$url"
}

lineage() {
    local out=$1 gki="$O/gki" m
    mkdir -p "$gki" "$out"
    [ -s "$gki/Image.gz" ] || ci_get Image.gz "$gki/Image.gz"
    if [ ! -d "$gki/system_dlkm" ]; then
        ci_get system_dlkm_staging_archive.tar.gz "$gki/system_dlkm.tar.gz"
        mkdir -p "$gki/system_dlkm" && tar xzf "$gki/system_dlkm.tar.gz" -C "$gki/system_dlkm"
    fi
    rm -f "$out"/*.ko
    cp "$gki/Image.gz" "$O/dist/mt6833.dtb" "$O/dist/dtbo.img" "$DM"/lineage/*.modules.load* "$out/"
    for m in $(cat "$DM/lineage/system_dlkm.modules.load"); do
        cp "$(find "$gki/system_dlkm" -name "$(basename "$m")" | head -1)" "$out/"
    done
    for m in $(cat "$DM"/lineage/vendor_*.modules.load* "$DM/lineage/vendor_dlkm.modules.extra" | sort -u); do
        cp "$O/dist/$(basename "$m")" "$out/"
    done
    echo "$(ls "$out"/*.ko | wc -l) modules, Image.gz, mt6833.dtb, dtbo.img in $out"
}

case "${1:-all}" in
    dtbs) fetch; [ -x "$O/scripts/dtc/dtc" ] || configure; dtbs ;;
    lineage) [ -n "${2:-}" ] || { echo "usage: $0 lineage DIR" >&2; exit 1; }
        fetch; configure; build; dist; dtbs; lineage "$(realpath -m "$2")" ;;
    *) fetch; configure; build; dist; dtbs ;;
esac
