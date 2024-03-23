CC = gcc
ARM_CC = arm-none-linux-gnueabihf-gcc
INCLUDE_DIR = ./include/
CGI_BIN_DIR = /home/jeremy/Documents/www/cgi-bin/
ROOTFS_DIR = /home/jeremy/rootfs/
CGI_SRC = ./cgisrc/cgic.c
CGI_FILES = login.cgi garden_stuff_view.cgi

all: server-arm64 $(CGI_FILES) client-arm mydev

server-arm64: ./src/server.c
	$(CC) -o $@ $^ -I $(INCLUDE_DIR) -lpthread

$(CGI_FILES): %.cgi: ./cgisrc/%.c $(CGI_SRC)
	$(CC) -o $@ $^ -I $(INCLUDE_DIR) -lsqlite3
	@mv $@ $(CGI_BIN_DIR)

client-arm: ./src/client.c
	$(ARM_CC) -o $@ $^ -I $(INCLUDE_DIR) -lpthread `pkg-config --cflags --libs sqlite3`
	@mv $@ $(ROOTFS_DIR)

.PHONY: mydev
mydev:
	make -C mydev/Ambient-light
	make -C mydev/led
	make -C mydev/temp-hume

install:
	make -C mydev/Ambient-light install
	make -C mydev/led	install
	make -C mydev/temp-hume	install

clean:
	rm -f $(BIN_DIR)server-arm64
	rm -f $(addprefix $(CGI_BIN_DIR), $(CGI_FILES))
	rm -f $(ROOTFS_DIR)client-arm
	make -C mydev/Ambient-light clean
	make -C mydev/led clean
	make -C mydev/temp-hume clean