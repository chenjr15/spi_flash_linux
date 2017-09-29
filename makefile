objects = main.o spidev_driver.o
cc = clang 
CFLAGS += -std=c99 -Wall -Werror -Wno-unused
Program = flashtool
flashtool : $(objects)
	$(cc) $(CFLAGS) -o $(Program) $(objects)
main.o : main.c spidev_driver.h

spidev_driver.o : spidev_driver.h spidev_driver.c

.PHONY : clean
clean:
	rm flashtool $(objects)