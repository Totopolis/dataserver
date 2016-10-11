mkdir build
cd build
cmake -G "Visual Studio 14 Win64" -DCMAKE_BOOST=ON -DBOOST_ROOT=C:\boost\boost_1_62_0 -DBOOST_LIBRARYDIR=C:\boost\stage\lib ..
