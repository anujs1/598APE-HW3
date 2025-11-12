FUNC := gcc
copt := -c
OBJ_DIR := ./bin/
FLAGS := -O3 -march=native -funroll-loops -flto -fno-math-errno -std=c11 -lm -g -Werror

C_FILES := $(wildcard src/*.c)
OBJ_FILES := $(addprefix $(OBJ_DIR),$(notdir $(C_FILES:.c=.obj)))

all:
	$(FUNC) ./main.c -o ./main.exe $(FLAGS)

clean:
	rm -f ./*.exe