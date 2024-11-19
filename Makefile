OBJ_DIR:=build
SRC_DIR:=source
HDR_DIR:=include

CC:=g++
CFLAGS := -g -D _DEBUG -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs -Wstack-protector -fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer -Wlarger-than=8192 -Wstack-usage=8192 -pie -fPIE -Werror=vla -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr
RELEASE_FLAGS := -DNDEBUG -O3 -std=c++17

BUILD := DEBUG

ifeq ($(BUILD),RELEASE)
	override CFLAGS := $(RELEASE_FLAGS)
endif

override CFLAGS += -I./$(HDR_DIR)


test.out: $(OBJ_DIR)/jsonParser.o $(OBJ_DIR)/test.o
	$(CC) $(CFLAGS) $^ -o $@

static: $(OBJ_DIR)/jsonParser.o
	mkdir -p $(OBJ_DIR)
	ar rcs $(OBJ_DIR)/libjsonParser.a $^

$(OBJ_DIR)/jsonParser.o: $(SRC_DIR)/jsonParser.c $(HDR_DIR)/jsonParser.h
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/test.o: $(SRC_DIR)/test.c $(HDR_DIR)/jsonParser.h
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm build/*
