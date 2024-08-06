DATA_FILES=info.json palette.png wizard-spritesheet-53.png tileset.png
CARTS=forgotten-runes-demo.sqfs
RIVEMU=rivemu
RIVEMU_RUN=rivemu -quiet -workspace -exec
RIVEMU_EXEC=rivemu -quiet -no-window -sdk -workspace -exec
ifneq (,$(wildcard /usr/sbin/riv-run))
	RIVEMU_RUN=
	RIVEMU_EXEC=
endif
CFLAGS=$(shell $(RIVEMU_EXEC) riv-opt-flags -Ospeed)

# Compile all cartridges
all: $(CARTS)

# Compile map
map.lua: map.tmx
	tiled $< --export-map $@
map.h: map.lua
	lua5.4 map_conv.lua > map.h

# Compile cartridge
%.elf: %.c
	$(RIVEMU_EXEC) "gcc $< -o $@ $(CFLAGS) && riv-strip $@"

# Compile final cartridge file
%.sqfs: %.elf $(DATA_FILES)
	$(RIVEMU_EXEC) "riv-mksqfs $(DATA_FILES) $< $@"

# Test
run-%:
	$(RIVEMU_RUN) riv-jit-c $*.c

# Test all
RUN_CARTS=$(patsubst %.c, run-%, $(wildcard *.c))
test: $(RUN_CARTS)

# Check code for syntax errors
lint:
	gcc -Wall -Wextra -fsyntax-only -fanalyzer -I../../libriv *.c
	clang-tidy *.c -- -I../../libriv

# Clean generated cartridges
clean:
	rm -f *.sqfs *.elf

clean-map-gen:
	rm -f map.lua map.h
