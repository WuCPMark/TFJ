OBJ = codegen.o graph.o scc.o toposort.o banerjee.o ast.o helper.o importxml.o 

CXXFLAGS = -g -c `llvm-config --cppflags` -fPIC -std=c++0x
LLVM_VERSION = $(shell llvm-config --version)

ifeq ($(LLVM_VERSION), 3.0)
	CXXFLAGS += -DUSE_LLVM_30
endif

ifeq ($(LLVM_VERSION), 3.1)
	CXXFLAGS += -DUSE_LLVM_30
endif

all: $(OBJ) standalone pycomp
	g++ -g $(OBJ) standalone.o -o codegen -lxerces-c  `llvm-config --ldflags --libs all` -ldl
	g++ -g $(OBJ) pycomp.o -shared  -lxerces-c  `llvm-config --ldflags --libs all` -ldl -Wl,-soname,"pycomp.so" -fPIC -o pycomp.so

standalone:	
	g++ $(CXXFLAGS) standalone.cpp

pycomp:
	g++ $(CXXFLAGS) pycomp.cpp -I/usr/include/python2.7/

%.o: %.cpp
	g++ $(CXXFLAGS) $< 

clean:
	rm -rf codegen $(OBJ) standalone.o pycomp.o pycomp.so

tags:
	etags `find . -name "*.c" -o -name "*.cpp" -o -name "*.h"`

rebuild: clean all