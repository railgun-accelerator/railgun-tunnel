CC = gcc
TARGET_CORE = railgun_core

INCLUDE := -I./include
CFLAGS := -Wall $(INCLUDE)
LDFLAGS := -lrt

ifeq ($(RELEASE),1)
    CFLAGS += -O3 -DNDEBUG
else
    CFLAGS += -g
endif

ifeq ($(BITS),32)
    CFLAGS += -m32
else
    ifeq ($(BITS),64)
        CFLAGS += -m64
    else
    endif
endif

objects := railgun_timer.o railgun_output.o railgun_input.o railgun_sack.o railgun_utils.o railgun_core.o

.PHONY:all
all:$(TARGET_CORE)

$(TARGET_CORE):$(objects)
	$(CC) $? -o $@ $(CFLAGS) $(LDFLAGS)
	
$(objects): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
	
.PHONY:clean
clean:
	-rm -f $(objects) $(TARGET_SERVER) $(TARGET_CLIENT)
