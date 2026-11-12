all:
# -o compiled/main puts the compiled exe into the compiled folder
# right after that compiles the main.cpp from the src (source) file

	g++ -I src/include -L src/lib \
	-o compiled/main \
	src/main.cpp \
	-lmingw32 -lSDL2main -lSDL2