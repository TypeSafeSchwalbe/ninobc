
OUT = ninobc
CC = cc
SRC_DIR = src
OBJ_DIR = obj

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SOURCES = $(call rwildcard, $(SRC_DIR), *.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
ifeq ($(OS), Windows_NT)
	mkdir $(dir $@) 2> NUL
else
	mkdir -p $(dir $@)
endif
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(OUT)

clean:
ifeq ($(OS), Windows_NT)
	rmdir /s /q $(OBJ_DIR)
	del /f /q $(OUT)
else
	rm -rf $(OBJ_DIR)
	rm -f $(OUT)
endif



