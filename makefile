objects = main.o spidev_driver.o
cc = clang 
CFLAGS +=   -Wall -Werror -Wno-unused
flashtool : $(objects)

main.o : main.c spidev_driver.h

spidev_driver.o : spidev_driver.h spidev_driver.c

.PHONY : clean
clean:
	rm flashtool $(objects)