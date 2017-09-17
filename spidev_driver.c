/*
spi flash reader & writer 
 */

#include "spidev_driver.h"


void pabort(const char *s)
{
	perror(s);
	abort();
}
const char *device = "/dev/spidev0.0";
uint32_t mode=3;
uint8_t bits = 8;
char *input_file;
char *output_file;
uint32_t speed = 500000;
uint16_t delay;
int verbose;
char* backup_file=0;

uint8_t default_tx[BUFFER_SIZE] = {
	0x9F, 0xFF, 0xFF, 0xFF, 0x48, 0xFF,
};

uint8_t default_rx[BUFFER_SIZE] = {0, };
char *input_tx;

void hex_dump(const void *src, size_t length, size_t line_size,
			 char *prefix)
{
	int i = 0;
	const unsigned char *address = src;
	const unsigned char *line = address;
	unsigned char c;

	printf("%s | ", prefix);
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" | ");  /* right close */
			while (line < address) {
				c = *line++;
				printf("%c", (c < 33 || c == 255) ? 0x2E : c);
			}
			printf("\n");
			if (length > 0)
				printf("%s | ", prefix);
		}
	}
}

/*
 *  Unescape - process hexadecimal escape character
 *	  converts shell input "\x23" -> 0x23
 */
int unescape(char *_dst, char *_src, size_t len)
{
	int ret = 0;
	int match;
	char *src = _src;
	char *dst = _dst;
	unsigned int ch;

	while (*src) {
		if (*src == '\\' && *(src+1) == 'x') {
			match = sscanf(src + 2, "%2x", &ch);
			if (!match)
				pabort("malformed input string");

			src += 4;
			*dst++ = (unsigned char)ch;
		} else {
			*dst++ = *src++;
		}
		ret++;
	}
	return ret;
}

void transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len)
{
	int ret;
	int out_fd;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = len,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

	if (verbose)
		hex_dump(tx, len, 32, "TX");

	if (output_file) {
		out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (out_fd < 0)
			pabort("could not open output file");

		ret = write(out_fd, rx, len);
		if (ret != len)
			pabort("not all bytes written to output file");

		close(out_fd);
	}

	if (verbose || !output_file)
		hex_dump(rx, len, 32, "RX");
}

void print_usage(const char *prog)
{
	printf("Usage: %s [-DsbdlHOLC3B]\n", prog);
	puts("  -D --device   device to use (default /dev/spidev0.0)\n"
		 "  -s --speed	max speed (Hz)\n"
		 "  -d --delay	delay (usec)\n"
		 "  -b --bpw	  bits per word\n"
		 "  -i --input	input data from a file (e.g. \"test.bin\")\n"
		 "  -o --output   output data to a file (e.g. \"results.bin\")\n"
		 "  -l --loop	 loopback\n"
		 "  -H --cpha	 clock phase\n"
		 "  -O --cpol	 clock polarity\n"
		 "  -L --lsb	  least significant bit first\n"
		 "  -C --cs-high  chip select active high\n"
		 "  -3 --3wire	SI/SO signals shared\n"
		 "  -v --verbose  Verbose (show tx buffer)\n"
		 "  -p			Send data (e.g. \"1234\\xde\\xad\")\n"
		 "  -N --no-cs	no chip select\n"
		 "  -R --ready	slave pulls low to pause\n"
		 "  -2 --dual	 dual transfer\n"
		 "  -4 --quad	 quad transfer\n"
		 "  -B  --backup backup whole chip to a file (eg. \"backup.bin\") \n");
	exit(1);
}

void parse_opts(int argc, char *argv[])
{
	while (1) {
		const struct option lopts[] = {
			{ "device",  1, 0, 'D' },
			{ "speed",   1, 0, 's' },
			{ "delay",   1, 0, 'd' },
			{ "bpw",	 1, 0, 'b' },
			{ "input",   1, 0, 'i' },
			{ "output",  1, 0, 'o' },
			{ "loop",	0, 0, 'l' },
			{ "cpha",	0, 0, 'H' },
			{ "cpol",	0, 0, 'O' },
			{ "lsb",	 0, 0, 'L' },
			{ "cs-high", 0, 0, 'C' },
			{ "3wire",   0, 0, '3' },
			{ "no-cs",   0, 0, 'N' },
			{ "ready",   0, 0, 'R' },
			{ "dual",	0, 0, '2' },
			{ "verbose", 0, 0, 'v' },
			{ "quad",	0, 0, '4' },
			{"backup", 0, 0, 'B'}
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "D:s:d:b:i:o:lHOLC3NR24p:vB:",
				lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'D':
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		case 'b':
			bits = atoi(optarg);
			break;
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'l':
			mode |= SPI_LOOP;
			break;
		case 'H':
			mode |= SPI_CPHA;
			break;
		case 'O':
			mode |= SPI_CPOL;
			break;
		case 'L':
			mode |= SPI_LSB_FIRST;
			break;
		case 'C':
			mode |= SPI_CS_HIGH;
			break;
		case '3':
			mode |= SPI_3WIRE;
			break;
		case 'N':
			mode |= SPI_NO_CS;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'R':
			mode |= SPI_READY;
			break;
		case 'p':
			input_tx = optarg;
			break;
		case '2':
			mode |= SPI_TX_DUAL;
			break;
		case '4':
			mode |= SPI_TX_QUAD;
			break;
		 case 'B':
			backup_file = optarg;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
	if (mode & SPI_LOOP) {
		if (mode & SPI_TX_DUAL)
			mode |= SPI_RX_DUAL;
		if (mode & SPI_TX_QUAD)
			mode |= SPI_RX_QUAD;
	}
}

void transfer_escaped_string(int fd, char *str)
{
	size_t size = strlen(str);
	uint8_t *tx;
	uint8_t *rx;

	tx = malloc(size);
	if (!tx)
		pabort("can't allocate tx buffer");

	rx = malloc(size);
	if (!rx)
		pabort("can't allocate rx buffer");

	size = unescape((char *)tx, str, size);
	transfer(fd, tx, rx, size);
	free(rx);
	free(tx);
}

void transfer_file(int fd, char *filename)
{
	ssize_t bytes;
	struct stat sb;
	int tx_fd;
	uint8_t *tx;
	uint8_t *rx;

	if (stat(filename, &sb) == -1)
		pabort("can't stat input file");

	tx_fd = open(filename, O_RDONLY);
	if (fd < 0)
		pabort("can't open input file");

	tx = malloc(sb.st_size);
	if (!tx)
		pabort("can't allocate tx buffer");

	rx = malloc(sb.st_size);
	if (!rx)
		pabort("can't allocate rx buffer");

	bytes = read(tx_fd, tx, sb.st_size);
	if (bytes != sb.st_size)
		pabort("failed to read input file");

	transfer(fd, tx, rx, sb.st_size);
	free(rx);
	free(tx);
	close(tx_fd);
}

int is_flash_busy( ){
	default_tx[0]=Read_SR1;
	default_tx[1]=Dummy;
	transfer(fd, default_tx,default_rx,2);
	
	return (default_rx[1]&0x01);
}


void backup_chip(int fd , char *out_file, unsigned int flash_size_byte){
	unsigned int data_counter=0;
	while(is_flash_busy( ));
	default_tx[0]=Read_Data;
	default_tx[1]=0;
	default_tx[2]=0;
	default_tx[3]=0;
	
	out_fd = open(output_file,  O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (out_fd < 0)
			pabort("could not open backup file");
	
	while(data_counter< flash_size_byte){
		transfer(fd, default_tx,default_rx,BUFFER_SIZE);
		ret = write(out_fd, default_rx, BUFFER_SIZE);
		if (ret != len)
		pabort("not all bytes written to backup file");
		data_counter+=BUFFER_SIZE;
		
		
	}
	
	close(out_fd);
	
	
	
}

