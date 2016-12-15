rm *.o
g++ -I/usr/local/include -F/Library/Frameworks -c *.cpp
g++ *.o -o build/tilemap -F/Library/Frameworks -framework sfml-window -framework sfml-graphics -framework sfml-audio -framework sfml-system
cd build
./tilemap
cd ..
