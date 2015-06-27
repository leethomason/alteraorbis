# Initial build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make clean
make
cd ..

# Resources
build/builder resin/default.xml res/lumos.db

# Set up everything
rm -rf AlteraOrbis

mkdir AlteraOrbis
mkdir AlteraOrbis/res

cp res/* AlteraOrbis/res
cp build/alteraorbis AlteraOrbis
cp README.txt AlteraOrbis

tar czf AlteraOrbisLinux_$1.tar.gz AlteraOrbis

# tar -zxvf AlteraOrbisLinux.tar.gz



