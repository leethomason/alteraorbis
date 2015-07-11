#!/bin/bash
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
cp README.md AlteraOrbis

# Go StackOverflow. Don't know how I would have ever
# figured this out without you.
ALTERA_VERSION=`awk '/define/ {print $3}' version.h | tr -d '"\r'`
echo "Version: $ALTERA_VERSION"

rm AlteraOrbisLinux_$ALTERA_VERSION.tar.gz
tar czf AlteraOrbisLinux_$ALTERA_VERSION.tar.gz AlteraOrbis

# tar -zxvf AlteraOrbisLinux.tar.gz



