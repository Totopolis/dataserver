if [ $# -eq 0 ] 
then
   echo "No arguments supplied"
   export CMAKE_BUILD_TYPE=Release
else
   export CMAKE_BUILD_TYPE=$1
fi

echo CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

export SCRIPTS_DIR=$(pwd)

if [ ! -d dataserver ]; then
echo "git clone https://github.com/Totopolis/dataserver.git"
git clone https://github.com/Totopolis/dataserver.git
fi

cd dataserver
git pull

mkdir -p build
cd build
cmake ..
make

cd ${SCRIPTS_DIR}

