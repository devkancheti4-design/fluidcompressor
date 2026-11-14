#!/bin/sh
# build_windows.sh -- cross-build fluid.exe (Windows x86-64) from macOS or Linux with clang and an lld-link linker.
# No Windows SDK, MSVC or MinGW: src/wincrt is a small C runtime written directly on kernel32.
# Linker: lld-link from LLVM, or Rust's bundled one (LLD="rust-lld -flavor link"). /Brepro makes the build reproducible.
set -e
cd "$(dirname "$0")"
if [ -z "$LLD" ]; then
  if command -v lld-link >/dev/null 2>&1; then LLD=lld-link
  else R=$(ls ~/.rustup/toolchains/*/lib/rustlib/*/bin/rust-lld 2>/dev/null | head -1); [ -n "$R" ] && LLD="$R -flavor link"; fi
fi
[ -n "$LLD" ] || { echo "need lld-link (LLVM) or rust-lld; set LLD=..."; exit 1; }
RES=$(clang -print-resource-dir)/include
mkdir -p build
WT="--target=x86_64-pc-windows-msvc -nostdlibinc -isystem src/wincrt/include -isystem $RES -Isrc/wincrt -fno-stack-protector -mno-stack-arg-probe"
clang $WT -std=c11 -O2 -fwrapv -w -DLOOP_LAW -DSHZX_NO_MAIN -Isrc -Isrc/codec -c src/fluid.c -o build/fluid.obj
clang $WT -std=c11 -O2 -w -ffreestanding -c src/wincrt/wincrt.c -o build/wincrt.obj
$LLD /lib /def:src/wincrt/kernel32.def /machine:x64 /out:build/kernel32.lib
$LLD /nologo /Brepro /subsystem:console /entry:mainCRTStartup /nodefaultlib /machine:x64 /stack:0x1000000,0x1000000 /out:fluid.exe build/fluid.obj build/wincrt.obj build/kernel32.lib
echo "built fluid.exe"
