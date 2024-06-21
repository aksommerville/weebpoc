all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-pointer-sign -Wno-parentheses
LD:=gcc
LDPOST:=

INDEXHTML:=out/index.html
ZIP:=out/weebpoc.zip
all:$(INDEXHTML) $(ZIP)

SRCFILES:=$(shell find src -type f)

CFILES_MIN:=$(filter src/min/%.c,$(SRCFILES))
OFILES_MIN:=$(patsubst src/min/%.c,mid/min/%.o,$(CFILES_MIN))
-include $(OFILES_MIN:.o=.d)
mid/%.o:src/%.c;$(PRECMD) $(CC) -o$@ $<
EXE_MIN:=out/min
$(EXE_MIN):$(OFILES_MIN);$(PRECMD) $(LD) -o$@ $^ $(LDPOST)

SRCFILES_WWW:=$(filter src/www/%,$(SRCFILES))
$(INDEXHTML):$(SRCFILES_WWW) $(EXE_MIN);$(PRECMD) $(EXE_MIN) -o$@ src/www/index.html
$(ZIP):$(INDEXHTML);$(PRECMD) zip -jqX $@ $^ && echo "$@: $$(du -sb src/www | cut -f1) => $$(stat -c%s $(INDEXHTML)) => $$(stat -c%s $@)"

# A second directory "smaller" is just like "www" but I'm stripping out all the test-game stuff, and will try to reduce the platform size.
# Goal is to have the platform building to under 5 kB, and the less the merrier.
# Right off the bat, with only assets removed: 3289. So. Mission accomplished. But let's go even smaller...
# With the stub game removed from Game and Video: 2867
# With some new inlining of constants: 2778
# Eliminate numeric keypad mapping, who uses that: 2744
# Shorten a few identifiers here and there: 2711
# A few more shortens, and drop redundant literal number digits: 2670
# Added sound effects, and some logic to test it: 2939
# With the test stuff removed: 2839
INDEXHTML_SMALLER:=out/smaller.html
ZIP_SMALLER:=out/smaller.zip
all:$(ZIP_SMALLER)
SRCFILES_SMALLER:=$(filter src/smaller/%,$(SRCFILES))
$(INDEXHTML_SMALLER):$(SRCFILES_SMALLER) $(EXE_MIN);$(PRECMD) $(EXE_MIN) -o$@ src/smaller/index.html
$(ZIP_SMALLER):$(INDEXHTML_SMALLER);$(PRECMD) zip -jqX $@ $^ && echo "$@: $$(du -sb src/smaller | cut -f1) => $$(stat -c%s $(INDEXHTML_SMALLER)) => $$(stat -c%s $@)"

# 'make run' to serve the input files, prefer for high-frequency dev work.
run:;http-server src/www -c-1

# 'make run-final' to serve the minified single file. Definitely test this thoroughly before shipping!
# Unlike the loose input files, you can also just load this with `file:` protocol.
run-final:$(INDEXHTML);http-server out -c-1

clean:;rm -rf mid out
