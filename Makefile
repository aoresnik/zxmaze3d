
# NDEBUG Disables some consistency checks and debug prints on start
ZCC_FLAGS = -DNDEBUG

all: zxmaze3d.tap test-span.tap test-fixed-math.tap

zxmaze3d.tap : zxmaze3d.c draw-utils.o map.o state.o render-eng1.o tables.o timing.o span.o fixed-math.o cmd.o irq.o

test-span.tap : test-span.c timing.o tables.o span.o

test-fixed-math.tap : test-fixed-math.c timing.o fixed-math.o tables.o

.PHONY: all clean

clean:
	rm zxmaze3d.tap test-fixed-math.tap test-span.tap zxmaze3d.bin test-fixed-math.bin test-span.bin *.o

%.tap : %.c
	zcc +zx ${ZCC_FLAGS} -zorg=34816 -O2 -m -o $(*F).bin $^ -lndos -lgen_math -lim2 -create-app

%.o : %.c
	zcc -c ${ZCC_FLAGS} -O2 -o $@ $^

