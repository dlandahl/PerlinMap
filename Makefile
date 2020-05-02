make:
	clang++ main.cc -o MapGame -lSDL2 -Ofast -g

emscripten:
	em++ main.cc -s USE_SDL=2 -std=c++17 -o mapgame.html

