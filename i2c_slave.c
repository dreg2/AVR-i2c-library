#include <avr/io.h>
#include <util/twi.h>
//#include <stdio.h>

#include "i2c.h"

// slave register array
volatile uint8_t *reg_addr;
volatile uint8_t *reg_array;
volatile uint8_t  reg_array_size;

//----------------------------------------------------------------------------------------------------
// start i2c slave
//----------------------------------------------------------------------------------------------------
void i2c_slave_start(uint8_t addr, uint8_t *index, uint8_t *array, uint8_t size)
	{
	// initialize register array pointers
	reg_addr       = index;
	reg_array      = array;
	reg_array_size = size;

	// load slave address into address register
	TWAR = (uint8_t)(addr << 1);

	// set slave in twcr with interrupt
	TWCR = _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
	}

//----------------------------------------------------------------------------------------------------
// stop i2c slave
//----------------------------------------------------------------------------------------------------
void i2c_slave_stop(void)
	{
	// clear acknowledge and enable bits
	TWCR &= (uint8_t)~(_BV(TWEA) | _BV(TWEN));
	}

//----------------------------------------------------------------------------------------------------
// i2c slave interrupt service routine
//----------------------------------------------------------------------------------------------------
ISR(TWI_vect)
	{
	PROVIDE_ENTRY(TWI_vect);
	// slave recieve states
	#define STATE_RECV_ADDR 0
	#define STATE_RECV_DATA 1
	static uint8_t recv_state = STATE_RECV_ADDR;

	switch (TW_STATUS)
		{
		//--------------------------------------------------
		// received slave address + receive flag
		//--------------------------------------------------
		case TW_SR_SLA_ACK:      // 0x60: SLA+R received, ACK returned
			{
			recv_state = STATE_RECV_ADDR;
			*reg_addr = 0;

			TWCR |= _BV(TWEA); // set to receive next byte with ACK
			}
			break;

		//--------------------------------------------------
		// received data and sent ACK
		//--------------------------------------------------
		case TW_SR_DATA_ACK:     // 0x80: data received, ACK returned
			{
			// first byte received is reg addr
			if (recv_state == STATE_RECV_ADDR)
				{
				recv_state = STATE_RECV_DATA;
				*reg_addr = TWDR;

				TWCR |= _BV(TWEA); // set to receive next byte with ACK
				}

			// second and subsequent bytes received are data
			else
				{
				// store the data at the current address
				reg_array[(*reg_addr)++] = TWDR;

				// check for register limit
				if (*reg_addr < (reg_array_size - 1))
					{
					TWCR |= _BV(TWEA); // set to receive next byte with ACK
					}
				else
					{
					TWCR &= (uint8_t)~_BV(TWEA); // set to receive next byte with NACK
					}
				}
			}
			break;

		//--------------------------------------------------
		// received data and sent NACK
		//--------------------------------------------------
		case TW_SR_DATA_NACK:    // 0x88: data received, NACK returned
			{
			reg_array[*reg_addr] = TWDR;
			recv_state = STATE_RECV_ADDR;
			*reg_addr = 0;
			TWCR |= _BV(TWEA); // set to receive next byte with ACK
			}
			break;


		//--------------------------------------------------
		// received slave address + transmit flag
		// received ACK from last transmit
		//--------------------------------------------------
		case TW_ST_SLA_ACK:      // 0xA8: SLA+W received, ACK returned
		case TW_ST_DATA_ACK:     // 0xB8: data transmitted, ACK received
			{
			// copy the register to transmit into twdr
			TWDR = reg_array[(*reg_addr)++];

			// check for register limit
			if (*reg_addr < reg_array_size)
				{
				TWCR |= _BV(TWEA); // set to receive next byte with ACK
				}
			else
				{
				TWCR &= (uint8_t)~_BV(TWEA); // set to receive next byte with NACK
				}
			}
			break;

		//--------------------------------------------------
		// transmitted data and NACK received
		// transmitted last data and ACK received
		// bus error
		// everything else
		//--------------------------------------------------
		case TW_ST_DATA_NACK:    // 0xC0: data transmitted, NACK received
		case TW_ST_LAST_DATA:    // 0xC8: last data byte transmitted, ACK received
			{
			// fresh start
			recv_state = STATE_RECV_ADDR;
			*reg_addr = 0;
			TWCR |= _BV(TWEA); // set to receive next byte with ACK
			}
			break;

		//--------------------------------------------------
		// received stop signal
		//--------------------------------------------------
		case TW_SR_STOP:         // 0xA0: stop condition received while selected
			{
			recv_state = STATE_RECV_ADDR;
			TWCR |= _BV(TWEA); // set to receive next byte with ACK
			}
			break;

		//--------------------------------------------------
		// bus error, unknown status
		//--------------------------------------------------
		default:                 // unknown status
			{
			// fresh start
			recv_state = STATE_RECV_ADDR;
			*reg_addr = 0;
			TWCR |= _BV(TWEA) | _BV(TWSTO); // release SCL and SDA
			}
			break;
		}

	// clear interrupt, set twi enable, set interrupt enable
	TWCR |= _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
	}

