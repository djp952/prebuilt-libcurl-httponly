# LIBCURL 7.77.0 (HTTP Only)
[https://github.com/curl/curl](https://github.com/curl/curl)   
  
**TARGETS**   
* linux-i686 (gcc-4.9)   
* linux-x86_64 (gcc-4.9)   
* linux-armel (gcc-4.9)   
* linux-armhf (gcc-4.9)   
* linux-aarch64 (gcc-4.9)   
* android-21-armeabi-v7a (ndk-r20b/api-21)   
* android-21-arm64-v8a (ndk-r20b/api-21)  
* android-21-x86 (ndk-r20b/api-21)  
* android-28-armeabi-v7a (ndk-r20b/api-28)   
* android-28-arm64-v8a (ndk-r20b/api-28)  
* android-28-x86 (ndk-r20b/api-28)  
* raspbian-armhf   
* osx-x86_64 (apple-darwin15)   
   
**BUILD ENVIRONMENT**  
* Windows 10 x64 20H2 (19042)   
* [Windows Subsystem for Linux](https://docs.microsoft.com/en-us/windows/wsl/install-win10) (WSL v1 recommended)   
* [WSL Ubuntu 18.04 LTS Distro](https://www.microsoft.com/store/productId/9N9TNGVNDL3Q)   
  
**REQUIRED: CONFIGURE UBUNTU ON WINDOWS**   
* Open "Ubuntu 18.04 LTS"   
```
sudo dpkg --add-architecture i386
sudo add-apt-repository 'deb http://archive.ubuntu.com/ubuntu/ xenial main universe'
sudo apt-get update
sudo apt-get install gcc-4.9 g++-4.9 libc6-dev:i386 libstdc++-4.9-dev:i386 lib32gcc-4.9-dev
sudo apt-get install gcc-4.9-arm-linux-gnueabihf g++-4.9-arm-linux-gnueabihf gcc-4.9-arm-linux-gnueabi g++-4.9-arm-linux-gnueabi gcc-4.9-aarch64-linux-gnu g++-4.9-aarch64-linux-gnu
sudo apt-get install autoconf libtool make p7zip-full python
```
   
**CONFIGURE ANDROID TOOLCHAINS**   
* Open "Ubuntu 18.04 LTS"   
```
wget https://dl.google.com/android/repository/android-ndk-r20b-linux-x86_64.zip
7z x android-ndk-r20b-linux-x86_64.zip
```
  
**CONFIGURE OSXCROSS CROSS-COMPILER**   
* Generate the MAC OSX 10.11 SDK Package for OSXCROSS by following the instructions provided at [PACKAGING THE SDK](https://github.com/tpoechtrager/osxcross#packaging-the-sdk).  The suggested version of Xcode to use when generating the SDK package is Xcode 7.3.1 (May 3, 2016).
* Open "Ubuntu 18.04 LTS"   
```
sudo apt-get install cmake clang llvm-dev libxml2-dev uuid-dev libssl-dev libbz2-dev zlib1g-dev
git clone https://github.com/tpoechtrager/osxcross --depth=1
cp {MacOSX10.11.sdk.tar.bz2} osxcross/tarballs/
UNATTENDED=1 osxcross/build.sh
osxcross/build_compiler_rt.sh
sudo mkdir -p /usr/lib/llvm-6.0/lib/clang/6.0.0/include
sudo mkdir -p /usr/lib/llvm-6.0/lib/clang/6.0.0/lib/darwin
sudo cp -rv $(pwd)/osxcross/build/compiler-rt/compiler-rt/include/sanitizer /usr/lib/llvm-6.0/lib/clang/6.0.0/include
sudo cp -v $(pwd)/osxcross/build/compiler-rt/compiler-rt/build/lib/darwin/*.a /usr/lib/llvm-6.0/lib/clang/6.0.0/lib/darwin
sudo cp -v $(pwd)/osxcross/build/compiler-rt/compiler-rt/build/lib/darwin/*.dylib /usr/lib/llvm-6.0/lib/clang/6.0.0/lib/darwin
```
   
**BUILD LIBCURL (linux-i686)**   
* Open "Ubuntu 18.04 LTS"   
```
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export CC=gcc-4.9
export AR=gcc-ar-4.9
export RANLIB=gcc-ranlib-4.9
export CPPFLAGS="-I$(pwd)/prebuilt-libz/linux-i686/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/linux-i686/lib" 
export CFLAGS="-m32 -I/usr/include/i386-linux-gnu"
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --host i686-pc-linux-gnu --with-pic --without-ssl --with-zlib --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   

**BUILD LIBCURL (linux-x86_64)**   
* Open "Ubuntu 18.04 LTS"   
```
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export CC=gcc-4.9
export AR=gcc-ar-4.9
export RANLIB=gcc-ranlib-4.9
export CPPFLAGS="-I$(pwd)/prebuilt-libz/linux-x86_64/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/linux-x86_64/lib" 
export CFLAGS="-I/usr/include/x86_64-linux-gnu"
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --with-pic --without-ssl --with-zlib --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (linux-armel)**   
* Open "Ubuntu 18.04 LTS"   
```
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export CC=arm-linux-gnueabi-gcc-4.9
export AR=arm-linux-gnueabi-gcc-ar-4.9
export RANLIB=arm-linux-gnueabi-gcc-ranlib-4.9
export CPPFLAGS="-I$(pwd)/prebuilt-libz/linux-armel/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/linux-armel/lib" 
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --with-pic --without-ssl --with-zlib --host=arm-linux-gnueabi --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (linux-armhf)**   
* Open "Ubuntu 18.04 LTS"   
```
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export CC=arm-linux-gnueabihf-gcc-4.9
export AR=arm-linux-gnueabihf-gcc-ar-4.9
export RANLIB=arm-linux-gnueabihf-gcc-ranlib-4.9
export CPPFLAGS="-I$(pwd)/prebuilt-libz/linux-armhf/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/linux-armhf/lib" 
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --with-pic --without-ssl --with-zlib --host=arm-linux-gnueabihf --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (linux-aarch64)**   
* Open "Ubuntu 18.04 LTS"   
```
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export CC=aarch64-linux-gnu-gcc-4.9
export AR=aarch64-linux-gnu-gcc-ar-4.9
export RANLIB=aarch64-linux-gnu-gcc-ranlib-4.9
export CPPFLAGS="-I$(pwd)/prebuilt-libz/linux-aarch64/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/linux-aarch64/lib" 
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --with-pic --without-ssl --with-zlib --host=aarch64-linux-gnu --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (android-21-armeabi-v7a)**
* Open "Ubuntu 18.04 LTS"   
```
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export TOOLCHAIN=$(pwd)/android-ndk-r20b/toolchains/llvm/prebuilt/linux-x86_64
export AR=$TOOLCHAIN/bin/arm-linux-androideabi-ar
export AS=$TOOLCHAIN/bin/arm-linux-androideabi-as
export CC=$TOOLCHAIN/bin/armv7a-linux-androideabi21-clang
export CXX=$TOOLCHAIN/bin/armv7a-linux-androideabi21-clang++
export LD=$TOOLCHAIN/bin/arm-linux-androideabi-ld
export RANLIB=$TOOLCHAIN/bin/arm-linux-androideabi-ranlib
export STRIP=$TOOLCHAIN/bin/arm-linux-androideabi-strip
export CPPFLAGS="-I$(pwd)/prebuilt-libz/android-21-armeabi-v7a/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/android-21-armeabi-v7a/lib" 
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --with-pic --without-ssl --with-zlib --host=arm-linux-androideabi --target=arm-linux-androideabi --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (android-21-arm64-v8a)**
* Open "Ubuntu 18.04 LTS"   
```
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export TOOLCHAIN=$(pwd)/android-ndk-r20b/toolchains/llvm/prebuilt/linux-x86_64
export AR=$TOOLCHAIN/bin/aarch64-linux-android-ar
export AS=$TOOLCHAIN/bin/aarch64-linux-android-as
export CC=$TOOLCHAIN/bin/aarch64-linux-android21-clang
export CXX=$TOOLCHAIN/bin/aarch64-linux-android21-clang++
export LD=$TOOLCHAIN/bin/aarch64-linux-android-ld
export RANLIB=$TOOLCHAIN/bin/aarch64-linux-android-ranlib
export STRIP=$TOOLCHAIN/bin/aarch64-linux-android-strip
export CPPFLAGS="-I$(pwd)/prebuilt-libz/android-21-arm64-v8a/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/android-21-arm64-v8a/lib" 
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --with-pic --without-ssl --with-zlib --host=aarch64-linux-android --target=aarch64-linux-android --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (android-21-x86)**
* Open "Ubuntu 18.04 LTS"   
```
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export TOOLCHAIN=$(pwd)/android-ndk-r20b/toolchains/llvm/prebuilt/linux-x86_64
export AR=$TOOLCHAIN/bin/i686-linux-android-ar
export AS=$TOOLCHAIN/bin/i686-linux-android-as
export CC=$TOOLCHAIN/bin/i686-linux-android21-clang
export CXX=$TOOLCHAIN/bin/i686-linux-android21-clang++
export LD=$TOOLCHAIN/bin/i686-linux-android-ld
export RANLIB=$TOOLCHAIN/bin/i686-linux-android-ranlib
export STRIP=$TOOLCHAIN/bin/i686-linux-android-strip
export CPPFLAGS="-I$(pwd)/prebuilt-libz/android-21-x86/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/android-21-x86/lib" 
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --with-pic --without-ssl --with-zlib --host=i686-linux-android --target=i686-linux-android --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (android-28-armeabi-v7a)**
* Open "Ubuntu 18.04 LTS"   
```
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export TOOLCHAIN=$(pwd)/android-ndk-r20b/toolchains/llvm/prebuilt/linux-x86_64
export AR=$TOOLCHAIN/bin/arm-linux-androideabi-ar
export AS=$TOOLCHAIN/bin/arm-linux-androideabi-as
export CC=$TOOLCHAIN/bin/armv7a-linux-androideabi28-clang
export CXX=$TOOLCHAIN/bin/armv7a-linux-androideabi28-clang++
export LD=$TOOLCHAIN/bin/arm-linux-androideabi-ld
export RANLIB=$TOOLCHAIN/bin/arm-linux-androideabi-ranlib
export STRIP=$TOOLCHAIN/bin/arm-linux-androideabi-strip
export CPPFLAGS="-I$(pwd)/prebuilt-libz/android-28-armeabi-v7a/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/android-28-armeabi-v7a/lib" 
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --with-pic --without-ssl --with-zlib --host=arm-linux-androideabi --target=arm-linux-androideabi --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (android-28-arm64-v8a)**
* Open "Ubuntu 18.04 LTS"   
```
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export TOOLCHAIN=$(pwd)/android-ndk-r20b/toolchains/llvm/prebuilt/linux-x86_64
export AR=$TOOLCHAIN/bin/aarch64-linux-android-ar
export AS=$TOOLCHAIN/bin/aarch64-linux-android-as
export CC=$TOOLCHAIN/bin/aarch64-linux-android28-clang
export CXX=$TOOLCHAIN/bin/aarch64-linux-android28-clang++
export LD=$TOOLCHAIN/bin/aarch64-linux-android-ld
export RANLIB=$TOOLCHAIN/bin/aarch64-linux-android-ranlib
export STRIP=$TOOLCHAIN/bin/aarch64-linux-android-strip
export CPPFLAGS="-I$(pwd)/prebuilt-libz/android-28-arm64-v8a/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/android-28-arm64-v8a/lib" 
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --with-pic --without-ssl --with-zlib --host=aarch64-linux-android --target=aarch64-linux-android --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (android-28-x86)**
* Open "Ubuntu 18.04 LTS"   
```
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export TOOLCHAIN=$(pwd)/android-ndk-r20b/toolchains/llvm/prebuilt/linux-x86_64
export AR=$TOOLCHAIN/bin/i686-linux-android-ar
export AS=$TOOLCHAIN/bin/i686-linux-android-as
export CC=$TOOLCHAIN/bin/i686-linux-android28-clang
export CXX=$TOOLCHAIN/bin/i686-linux-android28-clang++
export LD=$TOOLCHAIN/bin/i686-linux-android-ld
export RANLIB=$TOOLCHAIN/bin/i686-linux-android-ranlib
export STRIP=$TOOLCHAIN/bin/i686-linux-android-strip
export CPPFLAGS="-I$(pwd)/prebuilt-libz/android-28-x86/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/android-28-x86/lib" 
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --with-pic --without-ssl --with-zlib --host=i686-linux-android --target=i686-linux-android --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (raspbian-armhf)**   
Open "Ubuntu"   
```
git clone https://github.com/raspberrypi/tools.git raspberrypi --depth=1
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export PATH=$(pwd)/raspberrypi/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin:$PATH
export CC=arm-linux-gnueabihf-gcc
export AR=arm-linux-gnueabihf-gcc-ar
export RANLIB=arm-linux-gnueabihf-gcc-ranlib
export CPPFLAGS="-I$(pwd)/prebuilt-libz/raspbian-armhf/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/raspbian-armhf/lib" 
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --with-pic --without-ssl --with-zlib --host=arm-linux-gnueabihf --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (osx-x86_64)**   
* Open "Ubuntu 18.04 LTS"   
```
git clone https://github.com/curl/curl.git -b curl-7_77_0 --depth=1
git clone https://github.com/djp952/prebuilt-libz.git -b libz-1.2.11 --depth=1
export PATH=$(pwd)/osxcross/target/bin:$PATH
export CC=x86_64-apple-darwin15-clang
export AR=x86_64-apple-darwin15-ar
export RANLIB=x86_64-apple-darwin15-ranlib
export CFLAGS="-mmacosx-version-min=10.9 -stdlib=libc++"
export CPPFLAGS="-I$(pwd)/prebuilt-libz/osx-x86_64/include"
export LDFLAGS="-L$(pwd)/prebuilt-libz/osx-x86_64/lib" 
export LIBS=-ldl
cd curl
autoreconf -fi
./configure --with-pic --without-ssl --with-zlib --host=x86_64-apple-darwin15 --target=x86_64-apple-darwin15 --disable-shared --disable-dict --disable-ftp --disable-file --disable-gopher --disable-imap --disable-ldap --disable-ldaps --disable-mqtt --disable-pop3 --disable-rtsp --disable-smb --disable-smtp --disable-telnet --disable-tftp
OSXCROSS_NO_EXTENSION_WARNINGS=1 make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
## ADDITIONAL LICENSE INFORMATION
   
**XCODE AND APPLE SDKS AGREEMENT**   
The instructions provided above indirectly reference the use of intellectual material that is the property of Apple, Inc.  This intellectual material is not FOSS (Free and Open Source Software) and by using it you agree to be bound by the terms and conditions set forth by Apple, Inc. in the [Xcode and Apple SDKs Agreement](https://www.apple.com/legal/sla/docs/xcode.pdf).
