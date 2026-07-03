#!/usr/bin/env bash
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[0;33m'
NC='\033[0m'

print() {
  printf "%b\n" "$1"
}

detect_platform() {
  if [ -n "${PREFIX-}" ] && [ -d "/data/data/com.termux/files/home" ]; then
    echo "termux"
    return
  fi
  unameOut="$(uname -s 2>/dev/null || true)"
  case "${unameOut}" in
    Linux*) echo "linux" ;;
    Darwin*) echo "macos" ;;
    FreeBSD*) echo "freebsd" ;;
    *) echo "unknown" ;;
  esac
}

detect_pkg_mgr() {
  if command -v pkg >/dev/null 2>&1 && [ -d "/data/data/com.termux/files/home" ]; then
    echo "pkg"
    return
  fi
  if command -v apt-get >/dev/null 2>&1; then echo "apt"; return; fi
  if command -v dnf >/dev/null 2>&1; then echo "dnf"; return; fi
  if command -v yum >/dev/null 2>&1; then echo "yum"; return; fi
  if command -v pacman >/dev/null 2>&1; then echo "pacman"; return; fi
  if command -v apk >/dev/null 2>&1; then echo "apk"; return; fi
  if command -v brew >/dev/null 2>&1; then echo "brew"; return; fi
  if command -v zypper >/dev/null 2>&1; then echo "zypper"; return; fi
  echo "none"
}

install_packages() {
  mgr="$(detect_pkg_mgr)"
  deps=("$@")
  case "$mgr" in
    pkg)
      for d in "${deps[@]}"; do pkg install -y "$d" || true; done
      ;;
    apt)
      sudo apt-get update -y || true
      sudo apt-get install -y "${deps[@]}" || true
      ;;
    dnf)
      sudo dnf install -y "${deps[@]}" || true
      ;;
    yum)
      sudo yum install -y "${deps[@]}" || true
      ;;
    pacman)
      sudo pacman -S --noconfirm "${deps[@]}" || true
      ;;
    apk)
      sudo apk add "${deps[@]}" || true
      ;;
    brew)
      brew install "${deps[@]}" || true
      ;;
    zypper)
      sudo zypper install -y "${deps[@]}" || true
      ;;
    *)
      return 1
      ;;
  esac
}

choose_compiler_and_flags() {
  if command -v clang++ >/dev/null 2>&1; then
    CXX=clang++
  elif command -v g++ >/dev/null 2>&1; then
    CXX=g++
  else
    CXX=c++
  fi
  CXXFLAGS="-std=c++17 -O2 -pipe -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIC"
}

compile_vano() {
  builddir="$1"
  outbin="$2"
  
  if [ ! -d "src" ]; then
    print "${RED}error:${NC} cannot find 'src' directory. please run this script from the root of the vano repository."
    return 1
  fi

  mkdir -p "$builddir"
  files=(src/main.cpp src/editor.cpp src/buffer.cpp src/cursor.cpp src/screen.cpp src/input.cpp src/file.cpp src/utils.cpp)
  objs=()
  for f in "${files[@]}"; do
    if [ ! -f "$f" ]; then
      print "${YELLOW}warning:${NC} missing source $f, skipping"
      continue
    fi
    base="$(basename "$f" .cpp)"
    obj="$builddir/$base.o"
    "$CXX" $CXXFLAGS -c "$f" -o "$obj"
    objs+=("$obj")
  done
  if [ "${#objs[@]}" -eq 0 ]; then
    print "${RED}error:${NC} no object files produced"
    return 1
  fi
  "$CXX" "${objs[@]}" $CXXFLAGS -o "$outbin"
}

install_binary() {
  bin="$1"
  prefer_dest="$2"
  if [ -w "$prefer_dest" ] || { [ ! -e "$prefer_dest" ] && [ -w "$(dirname "$prefer_dest")" ]; }; then
    mv "$bin" "$prefer_dest"
    chmod +x "$prefer_dest"
    print "${GREEN}installed:${NC} $prefer_dest"
    return 0
  fi
  if command -v sudo >/dev/null 2>&1; then
    sudo mv "$bin" "$prefer_dest"
    sudo chmod +x "$prefer_dest"
    print "${GREEN}installed with sudo:${NC} $prefer_dest"
    return 0
  fi
  localuserbin="${HOME}/.local/bin"
  mkdir -p "$localuserbin"
  mv "$bin" "$localuserbin/vano"
  chmod +x "$localuserbin/vano"
  print "${GREEN}installed to user location:${NC} $localuserbin/vano"
  if ! echo "$PATH" | grep -q "$localuserbin"; then
    print "${YELLOW}hint:${NC} add $localuserbin to your PATH"
  fi
}

main() {
  print "${CYAN}Vano Text Editor Installer${NC}"
  platform="$(detect_platform)"
  print "platform: $platform"
  pkgmgr="$(detect_pkg_mgr)"
  print "package manager: $pkgmgr"
  case "$platform" in
    termux)
      deps=(git clang make tar)
      ;;
    macos)
      deps=(git make tar)
      ;;
    linux|freebsd|unknown)
      deps=(git g++ make tar)
      ;;
  esac
  missing=()
  for d in "${deps[@]}"; do
    if ! command -v "$d" >/dev/null 2>&1; then
      missing+=("$d")
    fi
  done
  if [ "${#missing[@]}" -ne 0 ]; then
    print "${YELLOW}Missing:${NC} ${missing[*]}"
    if [ "$pkgmgr" = "none" ]; then
      print "${YELLOW}Please install:${NC} ${missing[*]}"
    else
      print "${CYAN}Attempting to install:${NC} ${missing[*]}"
      case "$pkgmgr" in
        pkg) install_packages "${missing[@]}" || true ;;
        apt) install_packages build-essential "${missing[@]}" || true ;;
        dnf|yum) install_packages gcc-c++ make "${missing[@]}" || true ;;
        pacman) install_packages base-devel "${missing[@]}" || true ;;
        apk) install_packages build-base g++ "${missing[@]}" || true ;;
        brew) install_packages "${missing[@]}" || true ;;
        zypper) install_packages gcc-c++ "${missing[@]}" || true ;;
      esac
    fi
  fi
  choose_compiler_and_flags
  builddir="$(mktemp -d 2>/dev/null || mktemp -d -t vano_build)"
  repodir="$(mktemp -d 2>/dev/null || mktemp -d -t vano_repo)"
  trap 'rm -rf "$builddir" "$repodir" >/dev/null 2>&1 || true' EXIT
  print "${CYAN}Cloning Vano repository...${NC}"
  git clone https://github.com/therealviren/vano.git "$repodir"
  cd "$repodir"
  outbin="$(pwd)/vano.tmp.$$.bin"
  print "${CYAN}Compiling using $CXX${NC}"
  if compile_vano "$builddir" "$outbin"; then
    print "${GREEN}Build succeeded${NC}"
  else
    print "${RED}Build failed${NC}"
    exit 1
  fi
  if [ "$platform" = "termux" ]; then
    preferred_dest="${PREFIX:-/data/data/com.termux/files/usr}/bin/vano"
  else
    if [ -w "/usr/bin" ] || command -v sudo >/dev/null 2>&1; then
      preferred_dest="/usr/bin/vano"
    else
      preferred_dest="${HOME}/.local/bin/vano"
    fi
  fi
  print "${CYAN}Installing to $preferred_dest${NC}"
  install_binary "$outbin" "$preferred_dest"
  print "${GREEN}Installation finished${NC}"
  if ! command -v vano >/dev/null 2>&1; then
    if [ -f "$preferred_dest" ]; then
      print "${CYAN}Run:${NC} $preferred_dest"
    else
      print "${YELLOW}vano installed in user directory but not in PATH${NC}"
    fi
  else
    print "${GREEN}vano available in PATH${NC}"
  fi
}

main "$@"
