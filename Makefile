EXEC = -o out/gol.exe
SRC = src/gol.c src/hash.c
FLAGS_DEV = -g -Wall -Wextra -pedantic
#FLAGS_RELEASE = 

# RayLib
RAYLIB = -I src/raylib/include -L src/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm


db:
	gcc $(SRC) $(EXEC) $(FLAGS_DEV) $(RAYLIB)

