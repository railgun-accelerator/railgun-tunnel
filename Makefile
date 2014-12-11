CC = gcc
TARGET_SERVER = railgun_server
TARGET_CLIENT = railgun_client

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

objects := railgun_timer.o railgun_output.o railgun_input.o railgun_sack.o railgun_utils.o railgun_client.o railgun_server.o

object_server := railgun_utils.o railgun_timer.o railgun_output.o railgun_input.o railgun_sack.o railgun_server.o
object_client := railgun_utils.o railgun_timer.o railgun_output.o railgun_input.o railgun_client.o 

.PHONY:all
all:$(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_SERVER):$(object_server)
	$(CC) $? -o $@ $(CFLAGS) $(LDFLAGS)
	
$(TARGET_CLIENT):$(object_client)
	$(CC) $? -o $@ $(CFLAGS) $(LDFLAGS)
	
$(objects): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
	
.PHONY:clean
clean:
	-rm -f $(objects) $(TARGET_SERVER) $(TARGET_CLIENT)
