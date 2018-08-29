CF = -Wall --std=c++14 -Og -g -I/usr/include/glm -I/usr/include/SDL2
LF = -Wall --std=c++14 -lSDL2 -lSDL2_image -lSDL2_mixer -lsndfile

SRC_PATH=src

CXX = g++
SRC = $(wildcard $(SRC_PATH)/*.cpp)
OBJ = $(SRC:$(SRC_PATH)/%.cpp=obj/%.o)
TRG = fakesid

all: $(TRG)


obj/%.o: $(SRC_PATH)/%.cpp Makefile
	@mkdir -p obj/
	$(CXX) $(CF) $< -c -o $@


$(TRG): $(OBJ) Makefile
	$(CXX) $(OBJ) -o $@ $(LF)


clean:
	rm -rf obj/ $(TRG)

