# Compiler and flags
CXX = g++
SANITIZER = -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

CXXFLAGS = -I inc -I ../General -D _DEBUG -lncurses -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++   \
		  -Waggressive-loop-optimizations 														   \
          -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts 		   \
          -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal    \
          -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline 		   \
          -Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked 			   \
          -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo    \
          -Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn 				   \
          -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default 	   \
          -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast 			   \
          -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wno-narrowing 	   \
          -Wno-old-style-cast -Wno-varargs -Wstack-protector -fcheck-new -fsized-deallocation 	   \
          -fstack-protector -fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer 	   \
          -Wlarger-than=81920 -Wstack-usage=81920 -Werror=vla

# Object files
OBJECTS = \
		  ../General/objects/wrDotFile.o ../General/objects/wrPngFile.o ../General/objects/choose_color_and_name_byType.o \
		  ../General/objects/treeFuncs.o \
		   \
          obj/main.o obj/readFile.o obj/tokenizer.o obj/predprocessor.o
# ../General/objects/pushTree.o
# Default target
all: out

# Link
out: $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(CXXFLAGS) $(SANITIZER)

# Compile source files
obj/%.o: src/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) $(SANITIZER)

../General/objects/%.o: ../General/graphDump/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) $(SANITIZER)

../General/objects/%.o: ../General/programTree/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) $(SANITIZER)

../General/objects/%.o: ../General/treeTransfer/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) $(SANITIZER)

# Clean
clean:
	rm -rf out obj/*.o ../General/objects/*.o