/* SPDX-License-Identifier: CC0-1.0 */
/* "I2C Debug Port feature contributed by Intel Corporation" */

#define I2C_DEBUG_PORT_WRITE_COMMAND         0x00
#define I2C_DEBUG_PORT_READ_COMMAND          0x01
#define I2C_DEBUG_PORT_READY_TO_READ_COMMAND 0x02

#define I2C_DEBUG_PORT_COMMAND_BIT_POSITION 5
#define I2C_DEBUG_PORT_COMMAND_BIT_MASK     0x7
#define I2C_DEBUG_PORT_DATA_SIZE_BIT_MASK   0x1F

/*
 * Ringbuffer for the I2C debug port implementation
 */
#ifndef I2C_DEBUG_PORT_RINGBUFFER_SIZE
#define I2C_DEBUG_PORT_RINGBUFFER_SIZE 128
#endif
uint8_t i2c_debug_port_ringbuffer[I2C_DEBUG_PORT_RINGBUFFER_SIZE];
uint8_t i2c_debug_port_ringbuffer_read;
uint8_t i2c_debug_port_ringbuffer_write;
/* Exit sequence for the I2C debug port == ESC + [24~ == F12 */
const uint8_t i2c_debug_port_exit_sequence[] = { 0x1B, 0x5B, 0x32, 0x34, 0x7E };
uint8_t i2c_debug_port_exit_sequence_index;

bool i2c_debug_port_ringbuffer_put(const uint8_t data) {
  if (i2c_debug_port_ringbuffer_write == i2c_debug_port_ringbuffer_read) {
    return false;
  }
  i2c_debug_port_ringbuffer[i2c_debug_port_ringbuffer_write] = data;
  i2c_debug_port_ringbuffer_write++;
  if (i2c_debug_port_ringbuffer_write == I2C_DEBUG_PORT_RINGBUFFER_SIZE) {
    i2c_debug_port_ringbuffer_write = 0;
  }
  return true;
}

bool i2c_debug_port_ringbuffer_get(uint8_t *data) {
  uint8_t index;

  index = i2c_debug_port_ringbuffer_read + 1;
  if (index == I2C_DEBUG_PORT_RINGBUFFER_SIZE) {
    index = 0;
  }
  /* Check if buffer is empty */
  if (index == i2c_debug_port_ringbuffer_write) {
    return false;
  }
  i2c_debug_port_ringbuffer_read = index;
  *data = i2c_debug_port_ringbuffer[i2c_debug_port_ringbuffer_read];
  return true;
}

uint8_t i2c_debug_port_ringbuffer_count() {
  if (i2c_debug_port_ringbuffer_write > i2c_debug_port_ringbuffer_read) {
    return i2c_debug_port_ringbuffer_write - i2c_debug_port_ringbuffer_read - 1;
  } else {
    return (I2C_DEBUG_PORT_RINGBUFFER_SIZE - i2c_debug_port_ringbuffer_read) + i2c_debug_port_ringbuffer_write;
  }
}

bool i2c_debug_port_ringbuffer_check_for_exit(const uint8_t last_data) {
  if (i2c_debug_port_exit_sequence[i2c_debug_port_exit_sequence_index] == last_data) {
    i2c_debug_port_exit_sequence_index++;
    if (i2c_debug_port_exit_sequence_index >=
        (sizeof(i2c_debug_port_exit_sequence) / sizeof(i2c_debug_port_exit_sequence[0]))) {
      i2c_debug_port_exit_sequence_index = 0;
      return true;
    }
  } else {
    i2c_debug_port_exit_sequence_index = 0;
  }
  return false;
}
