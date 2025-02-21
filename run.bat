rm -f build/*.o build/vdeoManager
make
sleep 5
cd build
./VideoManager
cd ..
