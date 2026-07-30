#!/bin/sh
#
# Build and install CPython for Nintendo Switch homebrew into devkitPro
# portlibs. The installed library is for embedding in an NRO; the standard
# library is staged as a romfs tree that applications can copy into their
# project.

set -eu

usage() {
    cat <<'EOF'
Usage: Platforms/NX/build-portlibs.sh [options]

Options:
  --build-dir DIR        Build directory (default: build/nx-portlibs)
  --prefix DIR           portlibs prefix (default: $DEVKITPRO/portlibs/switch)
  --runtime-prefix PATH  Python runtime prefix on Switch (default: romfs:/python)
  --with-build-python PY Host Python matching this branch (default: auto)
  --jobs N               Parallel make jobs (default: nproc/getconf, or 1)
  --destdir DIR          Stage install under DIR instead of writing directly
  --root                 Ask for sudo and install with elevated permissions
  --clean                Remove the NX build directory before configuring
  --help                 Show this help

The script expects DEVKITPRO and DEVKITA64 to be set and devkitA64 to be
installed. It builds a native host Python first, then cross-builds CPython with
devkitA64 and installs headers, libpython, pkg-config files, and romfs assets.
EOF
}

die() {
    echo "error: $*" >&2
    exit 1
}

find_jobs() {
    if command -v nproc >/dev/null 2>&1; then
        nproc
    elif command -v getconf >/dev/null 2>&1; then
        getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1
    else
        echo 1
    fi
}

abs_path() {
    case "$1" in
        /*) printf '%s\n' "$1" ;;
        *) printf '%s/%s\n' "$(pwd)" "$1" ;;
    esac
}

srcdir=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build_dir=$srcdir/build/nx-portlibs
prefix=${DEVKITPRO:-}/portlibs/switch
runtime_prefix=romfs:/python
jobs=$(find_jobs)
destdir=
root=no
clean=no
build_python=

while [ "$#" -gt 0 ]; do
    case "$1" in
        --build-dir)
            shift
            [ "$#" -gt 0 ] || die "--build-dir requires a value"
            build_dir=$(abs_path "$1")
            ;;
        --prefix)
            shift
            [ "$#" -gt 0 ] || die "--prefix requires a value"
            prefix=$1
            ;;
        --runtime-prefix)
            shift
            [ "$#" -gt 0 ] || die "--runtime-prefix requires a value"
            runtime_prefix=$1
            ;;
        --with-build-python)
            shift
            [ "$#" -gt 0 ] || die "--with-build-python requires a value"
            build_python=$1
            ;;
        --jobs|-j)
            shift
            [ "$#" -gt 0 ] || die "--jobs requires a value"
            jobs=$1
            ;;
        --destdir)
            shift
            [ "$#" -gt 0 ] || die "--destdir requires a value"
            destdir=$(abs_path "$1")
            ;;
        --root)
            root=yes
            ;;
        --clean)
            clean=yes
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
    shift
done

run_install() {
    if [ "$root" = yes ]; then
        sudo "$@"
    else
        "$@"
    fi
}

install_stdin() {
    target=$1
    if [ "$root" = yes ]; then
        sudo tee "$target" >/dev/null
    else
        cat > "$target"
    fi
}

[ -n "${DEVKITPRO:-}" ] || die "DEVKITPRO is not set"
[ -n "${DEVKITA64:-}" ] || die "DEVKITA64 is not set"
[ -d "$DEVKITPRO/libnx" ] || die "libnx is not installed under $DEVKITPRO/libnx"

if [ "$root" = yes ]; then
    command -v sudo >/dev/null 2>&1 || die "--root requires sudo"
fi

for tool in aarch64-none-elf-gcc aarch64-none-elf-g++ \
            aarch64-none-elf-gcc-ar aarch64-none-elf-gcc-ranlib \
            aarch64-none-elf-readelf; do
    command -v "$tool" >/dev/null 2>&1 || die "$tool not found in PATH"
done

if command -v aarch64-none-elf-pkg-config >/dev/null 2>&1; then
    pkg_config=aarch64-none-elf-pkg-config
elif command -v pkg-config >/dev/null 2>&1; then
    pkg_config=pkg-config
else
    pkg_config=
fi

pkg_config_libdir=$prefix/lib/pkgconfig:$DEVKITPRO/portlibs/switch/lib/pkgconfig

pkg_config_exists() {
    [ -n "$pkg_config" ] || return 1
    env PKG_CONFIG_LIBDIR="$pkg_config_libdir" PKG_CONFIG_PATH= \
        PKG_CONFIG_SYSROOT_DIR= "$pkg_config" --exists "$@"
}

pkg_config_cflags() {
    [ -n "$pkg_config" ] || return 0
    env PKG_CONFIG_LIBDIR="$pkg_config_libdir" PKG_CONFIG_PATH= \
        PKG_CONFIG_SYSROOT_DIR= "$pkg_config" --cflags "$@" 2>/dev/null || true
}

pkg_config_libs() {
    [ -n "$pkg_config" ] || return 0
    env PKG_CONFIG_LIBDIR="$pkg_config_libdir" PKG_CONFIG_PATH= \
        PKG_CONFIG_SYSROOT_DIR= "$pkg_config" --libs --static "$@" 2>/dev/null || true
}

filter_setup_libs() {
    filtered=
    for flag in $*; do
        case "$flag" in
            -pthread) ;;
            *) filtered="$filtered $flag" ;;
        esac
    done
    printf '%s\n' "$filtered"
}

host_build_dir=$build_dir/build-python
nx_build_dir=$build_dir/nx

if [ "$clean" = yes ]; then
    rm -rf "$nx_build_dir"
fi

mkdir -p "$host_build_dir" "$nx_build_dir"

package_version=$(sed -n 's/^m4_define(\[PYTHON_VERSION\], \[\(.*\)\])$/\1/p' "$srcdir/configure.ac" | head -1)
if [ -z "$build_python" ]; then
    for candidate in "python$package_version" python3 python; do
        if command -v "$candidate" >/dev/null 2>&1; then
            candidate_path=$(command -v "$candidate")
            candidate_version=$("$candidate_path" -c 'import sys; print(f"{sys.version_info[0]}.{sys.version_info[1]}")')
            if [ "$candidate_version" = "$package_version" ]; then
                build_python=$candidate_path
                break
            fi
        fi
    done
fi

if [ -z "$build_python" ]; then
    if command -v gcc >/dev/null 2>&1 || command -v cc >/dev/null 2>&1 || command -v clang >/dev/null 2>&1; then
        if [ ! -x "$host_build_dir/python" ]; then
            echo "Configuring host build Python..."
            (cd "$host_build_dir" && "$srcdir/configure")
            echo "Building host build Python..."
            make -C "$host_build_dir" -j"$jobs"
        fi
        build_python=$host_build_dir/python
    else
        die "no Python $package_version build Python found and no native C compiler is available"
    fi
fi

echo "Using build Python: $build_python"

site=$srcdir/Platforms/NX/config.site-aarch64-none-elf
switch_portlib_packages="zlib liblzma libzstd mbedtls mbedx509 mbedcrypto"
switch_portlib_cflags=$(pkg_config_cflags $switch_portlib_packages)
switch_portlib_libs=$(pkg_config_libs $switch_portlib_packages)
switch_portlib_setup_libs=$(filter_setup_libs $switch_portlib_libs)
switch_support_defines=
switch_support_packages=$switch_portlib_packages
switch_curl_setup=

if pkg_config_exists libcurl; then
    switch_support_defines="-DHAVE_SWITCH_CURL"
    switch_support_packages="$switch_support_packages libcurl"
    switch_curl_setup="_switch_curl _switch_curl.c $(pkg_config_cflags libcurl) $(filter_setup_libs "$(pkg_config_libs libcurl)") -lnx"
else
    echo "switch-curl was not found by pkg-config; switch_curl will report curl as unavailable."
fi

switch_support_cflags=$(pkg_config_cflags $switch_support_packages)
switch_support_libs=$(pkg_config_libs $switch_support_packages)
switch_support_libs=$(filter_setup_libs $switch_support_libs)
switch_support_libs="$switch_support_libs -lnx"

switch_input_setup="_switch_input _switch_input.c -I$DEVKITPRO/libnx/include -lnx"
switch_support_defines="$switch_support_defines -DHAVE_SWITCH_INPUT"

cppflags="-D__SWITCH__ -I$DEVKITPRO/libnx/include -I$prefix/include"
cppflags="$cppflags -I$DEVKITPRO/portlibs/switch/include"
cppflags="$cppflags $switch_portlib_cflags"
cflags="-fPIE"
ldflags="-specs=$DEVKITPRO/libnx/switch.specs -L$DEVKITPRO/libnx/lib"
ldflags="$ldflags -L$prefix/lib -L$DEVKITPRO/portlibs/switch/lib"
ldflags="$ldflags -Wl,-z,notext"
libs="-lnx -lm $switch_portlib_libs"

echo "Configuring Nintendo Switch CPython..."
rm -f "$nx_build_dir/Modules/Setup.local"
(
    cd "$nx_build_dir"
    CONFIG_SITE=$site \
    CC=aarch64-none-elf-gcc \
    CXX=aarch64-none-elf-g++ \
    AR=aarch64-none-elf-gcc-ar \
    RANLIB=aarch64-none-elf-gcc-ranlib \
    READELF=aarch64-none-elf-readelf \
    PKG_CONFIG=$pkg_config \
    PKG_CONFIG_LIBDIR=$pkg_config_libdir \
    PKG_CONFIG_PATH= \
    PKG_CONFIG_SYSROOT_DIR= \
    CPPFLAGS=$cppflags \
    CFLAGS=$cflags \
    LDFLAGS=$ldflags \
    LIBS=$libs \
    "$srcdir/configure" \
        --host=aarch64-none-elf \
        --build="$("$srcdir/config.guess")" \
        --with-build-python="$build_python" \
        --prefix="$prefix" \
        --exec-prefix="$prefix" \
        host_prefix="$runtime_prefix" \
        host_exec_prefix="$runtime_prefix" \
        --disable-ipv6 \
        --disable-test-modules \
        --without-mimalloc \
        --without-ensurepip
)

cat > "$nx_build_dir/Modules/Setup.local" <<EOF
# Switch support modules. This is generated by Platforms/NX/build-portlibs.sh.
*static*
switch_support switch_support.c $switch_support_defines $switch_support_cflags $switch_support_libs
switch_ssl switch_ssl.c $switch_portlib_cflags $switch_portlib_setup_libs -lnx
$switch_curl_setup
$switch_input_setup
EOF

echo "Building Nintendo Switch CPython..."
make -C "$nx_build_dir" -j"$jobs"

install_dest=
if [ -n "$destdir" ]; then
    install_dest="DESTDIR=$destdir"
    run_install mkdir -p "$destdir"
fi

echo "Installing headers, stdlib, static library, and pkg-config files..."
if [ "$root" = yes ]; then
    echo "Requesting sudo for install writes..."
    sudo -v
fi
run_install make -C "$nx_build_dir" $install_dest libinstall inclinstall libainstall

version=$(sed -n 's/^VERSION=[[:space:]]*//p' "$nx_build_dir/Makefile" | head -1)
ldversion=$(sed -n 's/^LDVERSION=[[:space:]]*//p' "$nx_build_dir/Makefile" | head -1)
abi_flags=$(sed -n 's/^ABIFLAGS=[[:space:]]*//p' "$nx_build_dir/Makefile" | head -1)
case "$ldversion" in
    *'$('*) ldversion=${version}${abi_flags} ;;
esac
install_prefix=${destdir}${prefix}
pcdir=$install_prefix/lib/pkgconfig
runtime_root=$install_prefix/share/python-switch/romfs/python

run_install mkdir -p "$pcdir" "$runtime_root"
if [ "python-$ldversion.pc" != "python-$version.pc" ]; then
    run_install ln -sf "python-$ldversion.pc" "$pcdir/python-$version.pc"
fi
if [ "python-$ldversion-embed.pc" != "python-$version-embed.pc" ]; then
    run_install ln -sf "python-$ldversion-embed.pc" "$pcdir/python-$version-embed.pc"
fi
run_install ln -sf "python-$version.pc" "$pcdir/python3.pc"
run_install ln -sf "python-$version-embed.pc" "$pcdir/python3-embed.pc"

if [ -f "$install_prefix/lib/python$version/config-$ldversion/libpython$ldversion.a" ]; then
    run_install ln -sf "python$version/config-$ldversion/libpython$ldversion.a" \
        "$install_prefix/lib/libpython$ldversion.a"
fi
if [ -f "$nx_build_dir/Modules/_decimal/libmpdec/libmpdec.a" ]; then
    run_install cp "$nx_build_dir/Modules/_decimal/libmpdec/libmpdec.a" \
        "$install_prefix/lib/python$version/config-$ldversion/libmpdec.a"
fi
if [ -f "$nx_build_dir/Modules/expat/libexpat.a" ]; then
    run_install cp "$nx_build_dir/Modules/expat/libexpat.a" \
        "$install_prefix/lib/python$version/config-$ldversion/libpyexpat.a"
fi
for hacl_lib in \
    libHacl_HMAC.a \
    libHacl_Hash_BLAKE2.a \
    libHacl_Hash_SHA3.a \
    libHacl_Hash_SHA2.a \
    libHacl_Hash_SHA1.a \
    libHacl_Hash_MD5.a; do
    if [ -f "$nx_build_dir/Modules/_hacl/$hacl_lib" ]; then
        run_install cp "$nx_build_dir/Modules/_hacl/$hacl_lib" \
            "$install_prefix/lib/python$version/config-$ldversion/$hacl_lib"
    fi
done

embed_pc=$pcdir/python-$version-embed.pc
embed_private_libs="$switch_support_libs"
for hacl_lib in \
    libHacl_HMAC.a \
    libHacl_Hash_BLAKE2.a \
    libHacl_Hash_SHA3.a \
    libHacl_Hash_SHA2.a \
    libHacl_Hash_SHA1.a \
    libHacl_Hash_MD5.a; do
    if [ -f "$install_prefix/lib/python$version/config-$ldversion/$hacl_lib" ]; then
        embed_private_libs="\${libdir}/python$version/config-$ldversion/$hacl_lib $embed_private_libs"
    fi
done
if [ -f "$install_prefix/lib/python$version/config-$ldversion/libmpdec.a" ]; then
    embed_private_libs="\${libdir}/python$version/config-$ldversion/libmpdec.a $embed_private_libs"
fi
if [ -f "$install_prefix/lib/python$version/config-$ldversion/libpyexpat.a" ]; then
    embed_private_libs="\${libdir}/python$version/config-$ldversion/libpyexpat.a $embed_private_libs"
fi
embed_private_libs="$embed_private_libs -lm"
if [ -f "$embed_pc" ]; then
    run_install sed -i "s|^Libs.private:.*|Libs.private: $embed_private_libs|" "$embed_pc"
fi

run_install rm -rf "$runtime_root/lib"
run_install mkdir -p "$runtime_root/lib"
run_install cp -R "$install_prefix/lib/python$version" "$runtime_root/lib/"
run_install rm -rf \
    "$runtime_root/switch_curl.py" \
    "$runtime_root/lib/python$version/ensurepip" \
    "$runtime_root/lib/python$version/idlelib" \
    "$runtime_root/lib/python$version/test" \
    "$runtime_root/lib/python$version/tkinter" \
    "$runtime_root/lib/python$version/turtledemo" \
    "$runtime_root/lib/python$version/venv"

native_romfs_files=$(find "$runtime_root" -type f \( \
    -name '*.so' -o \
    -name '*.pyd' -o \
    -name '*.nro' -o \
    -name '*.nso' -o \
    -name '*.elf' \
    \) -print)
if [ -n "$native_romfs_files" ]; then
    printf '%s\n' "$native_romfs_files" >&2
    die "native executable Python payloads must not be staged into romfs; link them statically or package them in an executable title layout"
fi

install_stdin "$install_prefix/share/python-switch/README.md" <<EOF
# CPython for Nintendo Switch

This portlibs install provides libpython${version}${abi_flags}.a, headers, and
pkg-config metadata for embedding CPython in Nintendo Switch homebrew.

Runtime files are staged under:

    share/python-switch/romfs/python

Copy that \`python\` directory into your application's romfs. The interpreter
is configured to look for its standard library at:

    $runtime_prefix/lib/python$version

The romfs runtime tree is for readable Python files and data only. Native
Python extensions, native wheels, and any other executable code must not be
loaded from romfs. Link native modules into the application, or package native
code in an executable title layout such as ExeFS when using a title format that
supports it.

When linking an application, use:

    aarch64-none-elf-pkg-config --cflags --libs python-$version-embed

or, if your environment uses regular pkg-config:

    PKG_CONFIG_LIBDIR=$prefix/lib/pkgconfig pkg-config --cflags --libs python-$version-embed
EOF

echo "Installed CPython $version for Switch into $install_prefix"
echo "Runtime romfs tree: $install_prefix/share/python-switch/romfs/python"
