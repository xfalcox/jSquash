# using giflib from https://sourceforge.net/projects/giflib/
GIFLIB_URL = https://sourceforge.net/projects/giflib/files/giflib-5.2.2.tar.gz/download
GIFLIB_PACKAGE = node_modules/giflib.tar.gz

GIFLIB_DIR = node_modules/giflib
BUILD_DIR = node_modules/build

OUT_DEC_JS = dec/gif_dec.js
OUT_DEC_CPP = dec/gif_dec.cpp
OUT_DEC_WASM = dec/gif_dec.wasm
ENVIRONMENT = web,worker

PRE_JS = pre.js

GIFLIB_OUT = $(BUILD_DIR)/libgif.a

STACK_SIZE = 5242880
INITIAL_MEMORY_SIZE = 16777216

.PHONY: all clean

all: $(OUT_DEC_JS)

# Build giflib into a static archive
$(GIFLIB_OUT): $(GIFLIB_DIR)/gif_lib.h
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I $(GIFLIB_DIR) -c $(GIFLIB_DIR)/dgif_lib.c -o $(BUILD_DIR)/dgif_lib.o
	$(CC) $(CFLAGS) -I $(GIFLIB_DIR) -c $(GIFLIB_DIR)/gifalloc.c -o $(BUILD_DIR)/gifalloc.o
	$(CC) $(CFLAGS) -I $(GIFLIB_DIR) -c $(GIFLIB_DIR)/gif_err.c -o $(BUILD_DIR)/gif_err.o
	$(CC) $(CFLAGS) -I $(GIFLIB_DIR) -c $(GIFLIB_DIR)/openbsd-reallocarray.c -o $(BUILD_DIR)/openbsd-reallocarray.o
	$(AR) rcs $@ $(BUILD_DIR)/dgif_lib.o $(BUILD_DIR)/gifalloc.o $(BUILD_DIR)/gif_err.o $(BUILD_DIR)/openbsd-reallocarray.o

# Build the final decoder JS+WASM
$(OUT_DEC_JS): $(OUT_DEC_CPP) $(GIFLIB_OUT)
	$(CXX) \
		-I $(GIFLIB_DIR) \
		$(CXXFLAGS) \
		$(LDFLAGS) \
		--pre-js $(PRE_JS) \
		--bind \
		-s ERROR_ON_UNDEFINED_SYMBOLS=0 \
		-s ENVIRONMENT=$(ENVIRONMENT) \
		-s EXPORT_ES6=1 \
		-s DYNAMIC_EXECUTION=0 \
		-s MODULARIZE=1 \
		-s STACK_SIZE=$(STACK_SIZE) \
		-s INITIAL_MEMORY=$(INITIAL_MEMORY_SIZE) \
		-o $@ \
		$+

# Download and extract giflib
$(GIFLIB_PACKAGE):
	mkdir -p $(@D)
	curl -sL $(GIFLIB_URL) -o $@

$(GIFLIB_DIR)/gif_lib.h: $(GIFLIB_PACKAGE)
	mkdir -p $(@D)
	tar xzm --strip 1 -C $(@D) -f $(GIFLIB_PACKAGE)

clean:
	$(RM) $(OUT_DEC_JS) $(OUT_DEC_WASM)
	$(RM) -r $(BUILD_DIR)
