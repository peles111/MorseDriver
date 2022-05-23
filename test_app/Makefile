CC = ${CROSS_COMPILE}gcc
CXX = ${CROSS_COMPILE}gcc
LD = ${CROSS_COMPILE}gcc

CFLAGS = -Wall -O2
LDFLAGS = -static -lpthread

SRC = src
OUT_DIR = bin
BUILD_DIR = build

OUT_REL = ${OUT_DIR}/test_app
BUILD_OBJ = ${BUILD_DIR}/test_app.o


all: ${OUT_REL}

${OUT_REL}: ${BUILD_OBJ}
	${LD} -o $@ $< ${LDFLAGS}

${BUILD_OBJ}: ${SRC}/test.c | ${OUT_DIR} ${BUILD_DIR}
	${CC} ${CFLAGS} -c ${SRC}/test.c -o $@


${OUT_DIR}:
	mkdir -p $@
${BUILD_DIR}:
	mkdir -p $@

clean:
	rm -rf ${OUT_DIR} ${BUILD_DIR}

.PHONY: clean dirs bin build

