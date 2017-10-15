#searchWifi.sh works only on zSun change it with a script doing the same job
#this is for cross-compile
CC=~/openwrt-zsun-zsun2/staging_dir/toolchain-mips_mips32_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-gcc-4.8.3

#remember to upload coresponding lib @zSun:/lib
LIBS=-lpthread

RM=rm -f

all: wifi_check

wifi_check: wifi_check.c
	$(CC) $< -o $@ $(LIBS)
clean:
	$(RM) wifi_check
