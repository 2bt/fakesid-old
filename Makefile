CF = -Wall --std=c++14 -Og -g -I/usr/include/glm -I/usr/include/SDL2
LF = -Wall --std=c++14 -lSDL2 -lSDL2_image -lSDL2_mixer -lsndfile

CXX = g++
SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:src/%.cpp=obj/%.o)
TRG = fakesid

all: $(TRG)


obj/%.o: src/%.cpp Makefile
	@mkdir -p obj/
	$(CXX) $(CF) $< -c -o $@


$(TRG): $(OBJ) Makefile
	$(CXX) $(OBJ) -o $@ $(LF)


clean:
	rm -rf obj/ $(TRG)



#### android
NDK = $(realpath ../android-ndk-r16)

android:
	cd android-project && $(NDK)/ndk-build

android-install: android
	cd android-project && ant debug install

android-run: android-install
	cd android-project && adb shell am start -a android.intenon.MAIN -n com.sdl.fakesid/com.sdl.fakesid.MainActivity

