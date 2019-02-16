#include <avr/io.h>
#include <util/twi.h>
#include <util/delay.h>

#include "i2c.h"


//----------------------------------------------------------------------------------------------------
// configure i2c bus
//----------------------------------------------------------------------------------------------------
void i2c_conf_bus(unsigned long twi_freq, uint8_t pur_flag)
	{
	// turn on internal pull-up resistors on i2c lines
	DDRC  &= (uint8_t)~(_BV(PINC4) | _BV(PINC5));         // set data direction to input
	if (pur_flag == TWI_PUR_ON)
		PORTC |= _BV(PINC4) | _BV(PINC5);             // output 1 on pins to turn on pull-up resistors
	else
		PORTC &= (uint8_t)~(_BV(PINC4) | _BV(PINC5)); // output 0 on pins to turn off pull-up resistors

	// turn off prescaler
	TWSR &= (uint8_t)~(_BV(TWPS1) | _BV(TWPS0));

	// set twi bit rate register
	TWBR = (uint8_t)(((F_CPU / twi_freq) - 16) / 2);
	}

//----------------------------------------------------------------------------------------------------
// send byte to i2c bus
//----------------------------------------------------------------------------------------------------
uint8_t i2c_send(uint8_t data)
	{
	// send byte
	TWDR = data;
	TWCR = _BV(TWINT) | _BV(TWEN);
	loop_until_bit_is_set(TWCR, TWINT);

	return TW_STATUS;
	}

//----------------------------------------------------------------------------------------------------
// recv byte from i2c bus
//----------------------------------------------------------------------------------------------------
uint8_t i2c_recv(uint8_t ack_flag)
	{
	// receive byte
	if (ack_flag == I2C_ACK)
		// receive and send ACK
		TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
	else
		// receive and send NACK
		TWCR = _BV(TWINT) | _BV(TWEN);
	loop_until_bit_is_set(TWCR, TWINT);

	// return received byte
	return TWDR;
	}

//----------------------------------------------------------------------------------------------------
// start i2c master sequence
//----------------------------------------------------------------------------------------------------
uint8_t i2c_master_start(uint8_t addr, uint8_t rw_flag)
	{
	// send start signal
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
	loop_until_bit_is_set(TWCR, TWINT);

	// check for successful start
	if ((TW_STATUS != TW_START) && (TW_STATUS != TW_REP_START))
		return 1;

	// send slave address with r/w bit
	i2c_send((uint8_t)((addr<<1) | (rw_flag&0x01)));

	// check for slave device acknowledgement
	if ((TW_STATUS != TW_MT_SLA_ACK) && (TW_STATUS != TW_MR_SLA_ACK))
		{
		i2c_master_stop();
		return 1;
		}

	return 0;
	}

//----------------------------------------------------------------------------------------------------
// stop i2c sequence
//----------------------------------------------------------------------------------------------------
void i2c_master_stop(void)
	{
	// send stop signal
	TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);
	}

//----------------------------------------------------------------------------------------------------
// master write to i2c slave
//----------------------------------------------------------------------------------------------------
uint8_t i2c_master_write(uint8_t addr, const uint8_t *out_buffer, size_t out_length, uint8_t start_flag)
	{
	// start i2c sequence
	if (start_flag == I2C_SEQ_START_STOP || start_flag == I2C_SEQ_START)
		if (i2c_master_start(addr, TW_WRITE))
			return 1;

	// send out_buffer bytes
	for (size_t i = 0; i < out_length; i++)
		{
		i2c_send(out_buffer[i]);
		if (TW_STATUS != TW_MT_DATA_ACK)
			return 1;
		}

	// stop i2c sequence
	if (start_flag == I2C_SEQ_START_STOP || start_flag == I2C_SEQ_STOP)
		i2c_master_stop();

	return 0;
	}

//----------------------------------------------------------------------------------------------------
// master read from i2c slave
//----------------------------------------------------------------------------------------------------
uint8_t i2c_master_read(uint8_t addr, uint8_t *in_buffer, size_t in_length, uint8_t start_flag)
	{
	// start i2c sequence
	if (start_flag == I2C_SEQ_START_STOP || start_flag == I2C_SEQ_START)
		if (i2c_master_start(addr, TW_READ))
			return 1;

	// recieve in_buffer bytes
	if (in_length > 1)
		{
		for (size_t i = 0; i < (in_length - 1); i++)
			in_buffer[i] = i2c_recv(I2C_ACK);
		}

	// receive last byte with NACK
	in_buffer[in_length-1] = i2c_recv(I2C_NACK);

	// stop i2c sequence
	if (start_flag == I2C_SEQ_START_STOP || start_flag == I2C_SEQ_STOP)
		i2c_master_stop();

	return 0;
	}


//----------------------------------------------------------------------------------------------------
// master transfer with slave
//----------------------------------------------------------------------------------------------------
uint8_t i2c_master_xfer(uint8_t addr, const uint8_t *out_data, size_t out_length, uint8_t *in_data, size_t in_length)
	{
	// send out buffer
	if (out_data != NULL && out_length > 0)
		{
		// start i2c sequence for write
		if (i2c_master_start(addr, TW_WRITE))
			return 1;

		// send out buffer bytes
		for (size_t i = 0; i < out_length; i++)
			{
			i2c_send(out_data[i]);
			if (TW_STATUS != TW_MT_DATA_ACK)
				return 1;
			}
		}

	// recv in buffer
	if (in_data != NULL && in_length > 0)
		{
		// start i2c sequence for read
		if (i2c_master_start(addr, TW_READ))
			return 1;

		// recieve buffer bytes
		if (in_length > 1)
			{
			for (size_t i = 0; i < (in_length - 1); i++)
				in_data[i] = i2c_recv(I2C_ACK);
			}

		// receive last byte with NACK
		in_data[in_length-1] = i2c_recv(I2C_NACK);
		}

	// send i2c stop 
	if ((out_data != NULL && out_length > 0) || (in_data != NULL && in_length > 0))
		i2c_master_stop();

	return 0;
	}
