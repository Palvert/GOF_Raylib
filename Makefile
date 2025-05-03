# Define the executable file
EXEC = -o out/gof.exe

# Define the source files
SRC = src/gof.c src/hash.c

# Define any compile-time flags
FLAGS_H = -Wall -Wextra -pedantic  # Hard flags
FLAGS_E = -Wall -Wextra            # Easy flags

# RayLib
RAYLIB = -I src/raylib/include -L src/raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm


make:
	gcc $(SRC) $(EXEC) $(FLAGS_E) $(RAYLIB)

db:
	gcc $(SRC) $(EXEC) -g $(FLAGS_E) $(RAYLIB)

