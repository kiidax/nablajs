Nabla JS
========

Nabla JS is a small EMCAScript interpreter with straight-forward implementation. 

### Dependencies

* Boehm-GC 7.4.2
* libatomic_ops 7.4.0
* readline 6.2
* Bison 2.7
* Flex 2.5.37
* pcre

RPM packages

* autoconf automake flex bison
* readline-devel

### Building without pkgconfig

```
my_prefix=`pwd`/install
export PATH=$my_prefix/bin:$PATH
export LD_LIBRARY_PATH=$my_prefix/lib
cd libatomic_ops-*
./configure --prefix=$my_prefix
cd gc-*
ATOMIC_OPS_CFLAGS="-I$my_prefix/include" ATOMIC_OPS_LIBS="-L$my_prefix/lib -latomic_ops" ./configure --prefix=$my_prefix
mkdir m4
cp pkg.m4 m4
cd nabla
autoreconf --install -Im4
GC_CFLAGS="-I$my_prefix/include" GC_LIBS="-L$my_prefix/lib -lgc" ./configure --prefix=$my_prefix
```

### Building with pkg_config

```
my_prefix=`pwd`/install
export PATH=$my_prefix/bin:$PATH
export LD_LIBRARY_PATH=$my_prefix/lib
export PKG_CONFIG_PATH=$my_prefix/lib/pkgconfig
cd libatomic_ops-*
./configure --prefix=$my_prefix
cd gc-*
./configure --prefix=$my_prefix
cd nabla
autoreconf --install
CXXFLAGS="-g -Wall -std=c++11" ./configure --prefix=$my_prefix
```