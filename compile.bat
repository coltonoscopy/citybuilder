g++ *.cpp -m32 -Wall -pedantic -O0 -std=c++11 -I"C:\SFML-2.3.2\include" -L"C:\SFML-2.3.2\lib" -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -o build\tilemap.exe
cd build
tilemap.exe
cd ..
