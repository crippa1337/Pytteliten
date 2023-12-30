EXE          = pytteliten

SOURCES     := main.cpp

ifeq ($(MAKECMDGOALS),mini)
    SOURCES := pytteliten-mini.cpp
    EXE     := pytteliten-mini
endif


CXXFLAGS    := -stdlib=libc++ -O3 -std=c++20 -Wall -Wextra -pedantic -DNDEBUG -march=native

CXX         := clang++
SUFFIX      :=

# Detect Windows
ifeq ($(OS), Windows_NT)
    SUFFIX   := .exe
    CXXFLAGS += -static
else
	CXXFLAGS += -pthread
endif

OUT := $(EXE)$(SUFFIX)

all: $(EXE)

minify:
	python3 minifier/minifier.py

mini: minify $(EXE)


$(EXE) : $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SOURCES)

clean:
	rm -rf *.o
