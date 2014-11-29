CC = gcc
TARGET_SERVER = udp_server
TARGET_CLIENT = udp_client

INCLUDE := -I./include
CFLAGS := -Wall -g -O2 $(INCLUDE)
LDFLAGS := -pthread

objects := utils.o udp_client.o udp_server.o

object_server := utils.o udp_server.o
object_client := utils.o udp_client.o

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
