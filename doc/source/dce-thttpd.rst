This scenario use HTTP protocol using a real server and a real client.

We use 2 externals software to do this experience:

 1. thttpd which is a light weight HTTP server
 
 2. wget is a well known HTTP client.

The topology is simple we use 2 nodes linked by a point to point channel.

As you must know to use a real application under NS-3/DCE you need to recompile it. 
So below there is the recipes to compile the needed softwares for DCE usage.

THTTPD  -------------------------------------------------------------------------------------------------

Below there is the commands you may use to build thttpd :

wget http://www.acme.com/software/thttpd/thttpd-2.25b.tar.gz
tar xf thttpd-2.25b.tar.gz
cd thttpd-2.25b
./configure
patch -p1 < dce-thttpd.patch
CFLAGS=-fPIC LDFLAGS="-pie -rdynamic" make
cp thttpd $BASEDCE/build/bin_dce

WGET ----------------------------------------------------------------------------------------------------

Below there is the commands you may use to build wget:

echo to get wget sources you need wget binary :)
wget http://ftp.gnu.org/gnu/wget/wget-1.14.tar.gz
tar xf wget-1.14.tar.gz
cd wget-1.14/
CFLAGS=-fPIC LDFLAGS="-pie -rdynamic" ./configure --disable-opie  --disable-digest --disable-ntlm  --disable-largefile --disable-threads --disable-nls  --disable-rpath  --disable-iri --without-ssl --without-zlib  --without-libiconv-prefix --without-libintl-prefix --without-libpth-prefix  --without-included-regex
make
cp src/wget $BASEDCE/build/bin_dce

---------------------------------------------------------------------------------------------------------

Then to run the scenario you must provide some web pages to download at the root of the node 0 
at minimum you should provide a file named index.html under files-0
then you can launch the simulation by calling:

$BASEDCE/build/myscripts/httpd/bin/dce-httpd

the downloaded files can be found under files-1/10.1.1.1 directory

---

 
