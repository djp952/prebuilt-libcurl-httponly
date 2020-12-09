#!/bin/bash
#***************************************************************************
#                                  _   _ ____  _
#  Project                     ___| | | |  _ \| |
#                             / __| | | | |_) | |
#                            | (__| |_| |  _ <| |___
#                             \___|\___/|_| \_\_____|
#
# Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
#
# This software is licensed as described in the file COPYING, which
# you should have received as part of this distribution. The terms
# are also available at https://curl.se/docs/copyright.html.
#
# You may opt to use, copy, modify, merge, publish, distribute and/or sell
# copies of the Software, and permit persons to whom the Software is
# furnished to do so, under the terms of the COPYING file.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
###########################################################################
set -eo pipefail

./buildconf

if [ "$NGTCP2" = yes ]; then
  if [ "$TRAVIS_OS_NAME" = linux -a "$GNUTLS" ]; then
    cd $HOME
    git clone --depth 1 https://gitlab.com/gnutls/nettle.git
    cd nettle
    ./.bootstrap
    ./configure LDFLAGS="-Wl,-rpath,$HOME/ngbuild/lib" --disable-documentation --prefix=$HOME/ngbuild
    make
    make install

    cd $HOME
    git clone --depth 1 https://gitlab.com/gnutls/gnutls.git pgtls
    cd pgtls
    ./bootstrap
    ./configure PKG_CONFIG_PATH=$HOME/ngbuild/lib/pkgconfig LDFLAGS="-Wl,-rpath,$HOME/ngbuild/lib" --with-included-libtasn1 --with-included-unistring --disable-guile --disable-doc --prefix=$HOME/ngbuild
    make
    make install
  else
    cd $HOME
    git clone --depth 1 -b OpenSSL_1_1_1g-quic-draft-29 https://github.com/tatsuhiro-t/openssl possl
    cd possl
    ./config enable-tls1_3 --prefix=$HOME/ngbuild
    make
    make install_sw
  fi

  cd $HOME
  git clone --depth 1 https://github.com/ngtcp2/nghttp3
  cd nghttp3
  autoreconf -i
  ./configure --prefix=$HOME/ngbuild --enable-lib-only
  make
  make install

  cd $HOME
  git clone --depth 1 https://github.com/ngtcp2/ngtcp2
  cd ngtcp2
  autoreconf -i
  ./configure PKG_CONFIG_PATH=$HOME/ngbuild/lib/pkgconfig LDFLAGS="-Wl,-rpath,$HOME/ngbuild/lib" --prefix=$HOME/ngbuild --enable-lib-only
  make
  make install
fi

if [ "$TRAVIS_OS_NAME" = linux -a "$BORINGSSL" ]; then
  cd $HOME
  git clone --depth=1 https://boringssl.googlesource.com/boringssl
  cd boringssl
  CXX="g++" CC="gcc" cmake -H. -Bbuild -GNinja -DCMAKE_BUILD_TYPE=release -DBUILD_SHARED_LIBS=1
  cmake --build build
  mkdir lib
  cp ./build/crypto/libcrypto.so ./lib/
  cp ./build/ssl/libssl.so ./lib/
  echo "BoringSSL lib dir: "`pwd`"/lib"
  cmake --build build --target clean
  rm -f build/CMakeCache.txt
  CXX="g++" CC="gcc" cmake -H. -Bbuild -GNinja -DCMAKE_POSITION_INDEPENDENT_CODE=on
  cmake --build build
  export LIBS=-lpthread
fi

if [ "$TRAVIS_OS_NAME" = linux -a "$OPENSSL3" ]; then
  cd $HOME
  git clone --depth=1 https://github.com/openssl/openssl
  cd openssl
  ./config enable-tls1_3 --prefix=$HOME/openssl3
  make
  make install_sw
fi

if [ "$TRAVIS_OS_NAME" = linux -a "$LIBRESSL" ]; then
  cd $HOME
  git clone --depth=1 -b v3.1.4 https://github.com/libressl-portable/portable.git libressl-git
  cd libressl-git
  ./autogen.sh
  ./configure --prefix=$HOME/libressl
  make
  make install
fi

if [ "$TRAVIS_OS_NAME" = linux -a "$QUICHE" ]; then
  cd $HOME
  git clone --depth=1 --recursive https://github.com/cloudflare/quiche.git
  curl https://sh.rustup.rs -sSf | sh -s -- -y
  source $HOME/.cargo/env
  cd $HOME/quiche
  cargo build -v --release --features pkg-config-meta,qlog
  mkdir -v deps/boringssl/src/lib
  ln -vnf $(find target/release -name libcrypto.a -o -name libssl.a) deps/boringssl/src/lib/
fi

# Install common libraries.
# The library build directories are set to be cached by .travis.yml. If you are
# changing a build directory name below (eg a version change) then you must
# change it in .travis.yml `cache: directories:` as well.
if [ $TRAVIS_OS_NAME = linux ]; then
  if [ ! -e $HOME/wolfssl-4.4.0-stable/Makefile ]; then
    cd $HOME
    curl -LO https://github.com/wolfSSL/wolfssl/archive/v4.4.0-stable.tar.gz
    tar -xzf v4.4.0-stable.tar.gz
    cd wolfssl-4.4.0-stable
    ./autogen.sh
    ./configure --enable-tls13 --enable-all
    touch wolfssl/wolfcrypt/fips.h
    make
  fi

  cd $HOME/wolfssl-4.4.0-stable
  sudo make install

  if [ "$MESALINK" = "yes" ]; then
    if [ ! -e $HOME/mesalink-1.0.0/Makefile ]; then
      cd $HOME
      curl https://sh.rustup.rs -sSf | sh -s -- -y
      source $HOME/.cargo/env
      curl -LO https://github.com/mesalock-linux/mesalink/archive/v1.0.0.tar.gz
      tar -xzf v1.0.0.tar.gz
      cd mesalink-1.0.0
      ./autogen.sh
      ./configure --enable-tls13
      make
    fi
    cd $HOME/mesalink-1.0.0
    sudo make install

  fi

  if [ ! -e $HOME/nghttp2-1.39.2/Makefile ]; then
    cd $HOME
    curl -LO https://github.com/nghttp2/nghttp2/releases/download/v1.39.2/nghttp2-1.39.2.tar.gz
    tar -xzf nghttp2-1.39.2.tar.gz
    cd nghttp2-1.39.2
    CXX="g++-8" CC="gcc-8" CFLAGS="" LDFLAGS="" LIBS="" ./configure --disable-threads --enable-app
    make
  fi

  cd $HOME/nghttp2-1.39.2
  sudo make install
fi
