#ifndef I2C_H_
#define I2C_H_

#include <avr/interrupt.h>
#include <stdint.h>
#include <stddef.h>

#include "common.h"

// hardware
#define TWI_FREQ_100K 100000UL   // twi scl frequency 100 khz
#define TWI_FREQ_50K   50000UL   // twi scl frequency 50 khz
#define TWI_PRESCALER 1          // twi bit rate prescaler

#define I2C_START_DELAY_US 0        // delay after start in usecs
#define I2C_STOP_DELAY_US  0        // delay after stop in usecs
#define I2C_SEND_DELAY_US  0        // delay after byte send in usecs
#define I2C_RECV_DELAY_US  0        // delay after byte receive in usecs
#define I2C_XFER_DELAY_US  0        // delay between send and receive during xfer in usecs

// base functions
void    i2c_conf_bus(unsigned long twi_freq, uint8_t pur_flag);
uint8_t i2c_send(uint8_t data);
uint8_t i2c_recv(uint8_t ack_flag);

// pur flag values
#define TWI_PUR_OFF   0          // twi pull-up resistors off
#define TWI_PUR_ON    1          // twi pull-up resistors on

// ack flag values
#define I2C_NACK 0
#define I2C_ACK  1

// master functions
uint8_t i2c_master_start(uint8_t addr, uint8_t rw_flag);
void    i2c_master_stop(void);
uint8_t i2c_master_write(uint8_t addr, const uint8_t *out_buffer, size_t out_length, uint8_t start_flag);
uint8_t i2c_master_read(uint8_t addr, uint8_t *in_buffer, size_t in_length, uint8_t start_flag);
uint8_t i2c_master_xfer(uint8_t addr, const uint8_t *out_buffer, size_t out_length, uint8_t *in_buffer, size_t in_length);

// start flag values
#define I2C_SEQ_NONE       0
#define I2C_SEQ_START      1
#define I2C_SEQ_STOP       2
#define I2C_SEQ_START_STOP 3

// slave functions
void    i2c_slave_start(uint8_t address, uint8_t *index, uint8_t *array, uint8_t size);
void    i2c_slave_stop(void);
ISR(TWI_vect);

#endif // I2C_H_
