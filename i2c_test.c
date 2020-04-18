#include <avr/io.h>
#include <util/twi.h>

#include <util/delay.h>

#include <stdio.h>

#include "i2c.h"


//----------------------------------------------------------------------------------------------------
// test slave
//----------------------------------------------------------------------------------------------------
#ifdef TEST_SLAVE

#define TEST_ADDR 0x48

#include <uart.h>

int main(void)
	{
	uart_init_baud();           // initialize uart

	#define SLAVE_REG_SIZE 64
	uint8_t slave_idx;
	uint8_t slave_reg[SLAVE_REG_SIZE];

	// load register array
	for (int i = 0; i < SLAVE_REG_SIZE; i++)
		{
		slave_reg[i] = 0xA5;
		}

	i2c_slave_start(TEST_ADDR, &slave_idx, slave_reg, SLAVE_REG_SIZE); // start as slave with test address 
	sei();                      // enable interrupts
	REQUEST_ENTRY(TWI_vect);    // pull ISR from library

	while(1)
		{
		getchar();
		printf("slave_idx = 0x%02hx\n", slave_idx);
		for (int i = 0; i < SLAVE_REG_SIZE / 8; i++)
			{
			printf("0x%02hx:   ", i);
			for (int j = 0; j < 8; j++)
				printf("0x%02hx ", slave_reg[(i*8)+j]);
			printf("\n");
			}
		printf("\n");	
		}

	return 0;
	}

//#endif // TEST_SLAVE



//----------------------------------------------------------------------------------------------------
// test master
//----------------------------------------------------------------------------------------------------
#else // TEST_SLAVE
//#ifdef TEST_MASTER

#define TEST_ADDR 0x5A
//#define TEST_ADDR 0x48

#include <uart.h>

int main(void)
	{
	uint8_t buffer[64];

	uart_init_baud(); // initialize uart

	// initialize reg buffer
	for (uint8_t i = 0; i < 10; i++)
		buffer[i] = i;

	i2c_conf_bus(TWI_FREQ_100K, TWI_PUR_ON); // initialize i2c

	for (int i = 0; i < 10; i++)
		printf("0x%02hx ", buffer[i]);
	printf("\n\n");

	while(1)
		{
		uint8_t ret = 0;
		uint8_t start_idx = 0x00;

		printf("1 - write, 2 - read, 3 - scan, 4 - xfer\n");
		int option = getchar();

		switch(option)
			{
			case '1': // write to registers
				ret = i2c_master_write(TEST_ADDR, &start_idx, 1, I2C_SEQ_START);
				if (ret) printf("master_write 1 failed\n");
				ret = i2c_master_write(TEST_ADDR, buffer, 10, I2C_SEQ_STOP);
				if (ret) printf("master_write 2 failed\n");
				i2c_master_stop();

				break;

			case '2': // read from registers
				buffer[0] = 0x06;
				ret = i2c_master_write(TEST_ADDR, &buffer[0], 1, I2C_SEQ_START);
				if (ret) printf("master_write 1 failed\n");
//				_delay_us(200);
				ret = i2c_master_read(TEST_ADDR, &buffer[1], 3, I2C_SEQ_START_STOP);
				if (ret) printf("master_read 1 failed\n");

				buffer[4] = 0x07;
				ret = i2c_master_write(TEST_ADDR, &buffer[4], 1, I2C_SEQ_START);
				if (ret) printf("master_write 2 failed\n");
//				_delay_us(200);
				ret = i2c_master_read(TEST_ADDR, &buffer[5], 3, I2C_SEQ_START_STOP);
				if (ret) printf("master_read 2 failed\n");

				break;

			case '3': // perform i2c address scan
				for (uint8_t i = 0; i < 8; i++)
					{
					printf("0x%02hx:  ", i*16);
					for (uint8_t j = 0; j < 16; j++)
						{
						uint8_t idx = (uint8_t)((i * 16) + j);
						if ((idx) <= 0x07 || (idx) >= 0x78)
							printf("xx ");  // reserved addresses
						else if (i2c_master_start(idx, 1))
							printf("-- ");  // no response
						else
							printf("%02hx ", (idx)); // respons
						}
					printf("\n");
					}
				i2c_master_stop();

			case '4': // xfer from registers
				buffer[0] = 0x06;
				ret = i2c_master_xfer(TEST_ADDR, &buffer[0], 1, &buffer[1], 3);
				if (ret) printf("master_xfer 1 failed\n");
				buffer[4] = 0x07;
				ret = i2c_master_xfer(TEST_ADDR, &buffer[4], 1, &buffer[5], 3);
				if (ret) printf("master_xfer 2 failed\n");
				break;

			default:
				break;
			}

		for (int i = 0; i < 10; i++)
			printf("0x%02hx ", buffer[i]);
		printf("\n\n");	
		}

	return 0;
	}

#endif // TEST_MASTER


