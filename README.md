# LIBCURL 7.60.0 (HTTP Only)
[https://github.com/curl/curl](https://github.com/curl/curl)   
  
**TARGETS**   
* linux-i686 (gcc-4.9)   
* linux-x86_64 (gcc-4.9)   
* linux-armel (gcc-4.9)   
* linux-armhf (gcc-4.9)   
* linux-aarch64 (gcc-4.9)   
* android-armeabi-v7a (ndk-r12b/api-21)   
* android-arm64-v8a (ndk-r12b/api-21)  
* android-x86 (ndk-r12b/api-21)  
* osx-x86_64 (apple-darwin15)   
   
**BUILD ENVIRONMENT**  
* Windows 10 x64 1803 (17134) "April 2018 Update"   
* Windows Subsystem for Linux   
* [Ubuntu on Windows 16.04.4 LTS](https://www.microsoft.com/store/productId/9NBLGGH4MSV6)   
* OSXCROSS Cross-Compiler (with Mac OSX 10.11 SDK)   
  
**CONFIGURE UBUNTU ON WINDOWS**   
Open "Ubuntu"   
```
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install gcc-4.9 g++-4.9 libc6-dev:i386 libstdc++-4.9-dev:i386 lib32gcc-4.9-dev 
sudo apt-get install gcc-4.9-arm-linux-gnueabihf g++-4.9-arm-linux-gnueabihf gcc-4.9-arm-linux-gnueabi g++-4.9-arm-linux-gnueabi gcc-4.9-aarch64-linux-gnu g++-4.9-aarch64-linux-gnu
sudo apt-get install autoconf libtool make p7zip-full python
```
   
**CONFIGURE ANDROID TOOLCHAINS**   
Open "Ubuntu"   
```
wget https://dl.google.com/android/repository/android-ndk-r16b-linux-x86_64.zip
7z x android-ndk-r16b-linux-x86_64.zip
android-ndk-r16b/build/tools/make_standalone_toolchain.py --arch arm --api 21 --stl libc++ --install-dir ./arm-linux-androideabi
android-ndk-r16b/build/tools/make_standalone_toolchain.py --arch arm64 --api 21 --stl libc++ --install-dir ./aarch64-linux-android
android-ndk-r16b/build/tools/make_standalone_toolchain.py --arch x86 --api 21 --stl libc++ --install-dir ./i686-linux-android
```
  
**CONFIGURE OSXCROSS CROSS-COMPILER**   
* Generate the MAC OSX 10.11 SDK Package for OSXCROSS by following the instructions provided at [PACKAGING THE SDK](https://github.com/tpoechtrager/osxcross#packaging-the-sdk).  The suggested version of Xcode to use when generating the SDK package is Xcode 7.3.1 (May 3, 2016).
* Open "Ubuntu on Windows"   
```
sudo apt-get install make clang zlib1g-dev libmpc-dev libmpfr-dev libgmp-dev
git clone https://github.com/tpoechtrager/osxcross --depth=1
cp {MacOSX10.11.sdk.tar.bz2} osxcross/tarballs/
UNATTENDED=1 osxcross/build.sh
GCC_VERSION=4.9.3 osxcross/build_gcc.sh
```
   
**BUILD LIBCURL (linux-i686)**   
Open "Ubuntu on Windows"   
```
git clone https://github.com/curl/curl.git -b curl-7_60_0 --depth=1
export CC=gcc-4.9
export AR=gcc-ar-4.9
export RANLIB=gcc-ranlib-4.9
export CFLAGS="-m32 -I/usr/include/i386-linux-gnu"
export LIBS=-ldl
cd curl
./buildconf
./configure --host i686-pc-linux-gnu --with-pic --without-ssl --without-zlib --disable-shared --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-telnet --disable-dict --disable-file --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   

**BUILD LIBCURL (linux-x86_64)**   
Open "Ubuntu on Windows"   
```
git clone https://github.com/curl/curl.git -b curl-7_60_0 --depth=1
export CC=gcc-4.9
export AR=gcc-ar-4.9
export RANLIB=gcc-ranlib-4.9
export CFLAGS="-I/usr/include/x86_64-linux-gnu"
export LIBS=-ldl
cd curl
./buildconf
./configure --with-pic --without-ssl --without-zlib --disable-shared --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-telnet --disable-dict --disable-file --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (linux-armel)**   
Open "Ubuntu on Windows"   
```
git clone https://github.com/curl/curl.git -b curl-7_60_0 --depth=1
export CC=arm-linux-gnueabi-gcc-4.9
export AR=arm-linux-gnueabi-gcc-ar-4.9
export RANLIB=arm-linux-gnueabi-gcc-ranlib-4.9
export LIBS=-ldl
cd curl
./buildconf
./configure --with-pic --without-ssl --without-zlib --host=arm-linux-gnueabi --disable-shared --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-telnet --disable-dict --disable-file --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (linux-armhf)**   
Open "Ubuntu on Windows"   
```
git clone https://github.com/curl/curl.git -b curl-7_60_0 --depth=1
export CC=arm-linux-gnueabihf-gcc-4.9
export AR=arm-linux-gnueabihf-gcc-ar-4.9
export RANLIB=arm-linux-gnueabihf-gcc-ranlib-4.9
export LIBS=-ldl
cd curl
./buildconf
./configure --with-pic --without-ssl --without-zlib --host=arm-linux-gnueabihf --disable-shared --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-telnet --disable-dict --disable-file --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (linux-aarch64)**   
Open "Ubuntu on Windows"   
```
git clone https://github.com/curl/curl.git -b curl-7_60_0 --depth=1
export CC=aarch64-linux-gnu-gcc-4.9
export AR=aarch64-linux-gnu-gcc-ar-4.9
export RANLIB=aarch64-linux-gnu-gcc-ranlib-4.9
export LIBS=-ldl
cd curl
./buildconf
./configure --with-pic --without-ssl --without-zlib --host=aarch64-linux-gnu --disable-shared --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-telnet --disable-dict --disable-file --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (android-armeabi-v7a)**
Open "Ubuntu on Windows"   
```
git clone https://github.com/curl/curl.git -b curl-7_60_0 --depth=1
export PATH=$(pwd)/arm-linux-androideabi/bin:$PATH
export CROSS_COMPILE=arm-linux-androideabi-
export CFLAGS="-D__ANDROID_API__=21"
export LIBS=-ldl
cd curl
./buildconf
./configure --with-pic --without-ssl --without-zlib --host=arm-linux-androideabi --target=arm-linux-androideabi --disable-shared --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-telnet --disable-dict --disable-file --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (android-arm64-v8a)**
Open "Ubuntu on Windows"   
```
git clone https://github.com/curl/curl.git -b curl-7_60_0 --depth=1
export PATH=$(pwd)/aarch64-linux-android/bin:$PATH
export CROSS_COMPILE=aarch64-linux-android-
export CFLAGS="-D__ANDROID_API__=21"
export LIBS=-ldl
cd curl
./buildconf
./configure --with-pic --without-ssl --without-zlib --host=aarch64-linux-android --target=aarch64-linux-android --disable-shared --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-telnet --disable-dict --disable-file --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (android-x86)**
Open "Ubuntu on Windows"   
```
git clone https://github.com/curl/curl.git -b curl-7_60_0 --depth=1
export PATH=$(pwd)/i686-linux-android/bin:$PATH
export CROSS_COMPILE=i686-linux-android-
export CFLAGS="-D__ANDROID_API__=21"
export LIBS=-ldl
cd curl
./buildconf
./configure --with-pic --without-ssl --without-zlib --host=i686-linux-android --target=i686-linux-android --disable-shared --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-telnet --disable-dict --disable-file --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (raspbian-armhf)**   
Open "Ubuntu on Windows"   
```
git clone https://github.com/raspberrypi/tools.git raspberrypi --depth=1
git clone https://github.com/curl/curl.git -b curl-7_60_0 --depth=1
export PATH=$(pwd)/raspberrypi/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin:$PATH
export CC=arm-linux-gnueabihf-gcc
export AR=arm-linux-gnueabihf-gcc-ar
export RANLIB=arm-linux-gnueabihf-gcc-ranlib
export LIBS=-ldl
cd curl
./buildconf
./configure --with-pic --without-ssl --without-zlib --host=arm-linux-gnueabihf --disable-shared --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-telnet --disable-dict --disable-file --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher
make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
**BUILD LIBCURL (osx-x86_64)**   
Open "Ubuntu on Windows"   
```
git clone https://github.com/curl/curl.git -b curl-7_60_0 --depth=1
export PATH=$(pwd)/osxcross/target/bin:$PATH
export CROSS_COMPILE=x86_64-apple-darwin15-
export CFLAGS="-mmacosx-version-min=10.7 -stdlib=libc++"
export LIBS=-ldl
cd curl
./buildconf
./configure --with-pic --without-ssl --without-zlib --host=x86_64-apple-darwin15 --target=x86_64-apple-darwin15 --disable-shared --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-telnet --disable-dict --disable-file --disable-tftp --disable-rtsp --disable-pop3 --disable-imap --disable-smtp --disable-gopher
OSXCROSS_NO_EXTENSION_WARNINGS=1 make
```
Get header files from include/curl   
Get libcurl.a from lib/.libs   
   
## ADDITIONAL LICENSE INFORMATION
   
**XCODE AND APPLE SDKS AGREEMENT**   
The instructions provided above indirectly reference the use of intellectual material that is the property of Apple, Inc.  This intellectual material is not FOSS (Free and Open Source Software) and by using it you agree to be bound by the terms and conditions set forth by Apple, Inc. in the [Xcode and Apple SDKs Agreement](https://www.apple.com/legal/sla/docs/xcode.pdf).
