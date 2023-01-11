/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <Arduino.h>
#include <Wire.h>

#define I2C_DEBUG_PORT_DEVICE_ADDRESS 0x6F

// FIXME: Cannot exceed `sizeof (uint8_t)` yet
#define I2C_DEBUG_PORT_RINGBUFFER_SIZE 128
#include "BusPirateRingbuffer.h"

void receive_on_arrival(int howMany)
{
	uint8_t raw_data, command, size;

	// No response to `READ` command's STOP
	if (howMany == 0)
		return;

	raw_data = Wire.read();
	command = (raw_data >> I2C_DEBUG_PORT_COMMAND_BIT_POSITION)
		  & I2C_DEBUG_PORT_COMMAND_BIT_MASK;
	size = raw_data & I2C_DEBUG_PORT_DATA_SIZE_BIT_MASK;

	switch (command) {
	case I2C_DEBUG_PORT_WRITE_COMMAND:
		/* Drain bytes from `rxBuffer` to print immediately */
		while (size--)
			Serial.write(Wire.read());
		break;
	}
}

void send_on_request(void)
{
	uint8_t raw_data, command, size;

	// `READ` commands begin with I2C write, but this is I2C index data
	raw_data = Wire.read();
	command = (raw_data >> I2C_DEBUG_PORT_COMMAND_BIT_POSITION)
		  & I2C_DEBUG_PORT_COMMAND_BIT_MASK;
	size = raw_data & I2C_DEBUG_PORT_DATA_SIZE_BIT_MASK;

	switch (command) {
	case I2C_DEBUG_PORT_READ_COMMAND:
		/* Queue bytes to `txBuffer` for transfer */
		while (size--)
			if (i2c_debug_port_ringbuffer_get(&raw_data))
				Wire.write(raw_data);
		break;
	case I2C_DEBUG_PORT_READY_TO_READ_COMMAND:
		/* Queue buffer size to `txBuffer` for transfer */
		Wire.write(i2c_debug_port_ringbuffer_count());
		break;
	}
}

void setup()
{
	// Block until user connects
	while (!Serial);

	/* Startup host-side (USB controller) first to notify user */
	// Baud rate is no-care for USB CDC/ACM
	Serial.begin(115200);

	Serial.println("I2C Debug Port (Adafruit-Arduino implementation)");
	Serial.println("Press F12 to exit");

	/* Startup target-side now (I2C controller) */
	// Connect to I2C bus as HDMI debug port (address)
	Wire.begin(I2C_DEBUG_PORT_DEVICE_ADDRESS);
	Wire.onReceive(receive_on_arrival);
	Wire.onRequest(send_on_request);

	i2c_debug_port_ringbuffer_read = 0;
	i2c_debug_port_ringbuffer_write = 1;
	i2c_debug_port_exit_sequence_index = 0;
}

void loop()
{
	uint8_t data_value;

	if (Serial.available()) {
		data_value = Serial.read();
		i2c_debug_port_ringbuffer_put(data_value);
		if (i2c_debug_port_ringbuffer_check_for_exit(data_value))
			Serial.println("@todo: User requests exit");
	}

	return;
}
