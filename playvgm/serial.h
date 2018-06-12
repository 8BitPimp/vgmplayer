/*                     .__       .__
 *   ______ ___________|__|____  |  |
 *  /  ___// __ \_  __ \  \__  \ |  |
 *  \___ \\  ___/|  | \/  |/ __ \|  |__
 * /____  >\___  >__|  |__(____  /____/
 *      \/     \/              \/
 */
#ifndef FILE_SERIAL_H
#define FILE_SERIAL_H

#include <stdint.h>

typedef struct serial_t serial_t;

#define SERIAL_COM(NUM) NUM

/* COM port helper enumeration
 */
enum {
  SERIAL_COM1 = 1,
  SERIAL_COM2 = 2,
  SERIAL_COM3 = 3,
  SERIAL_COM4 = 4,
  SERIAL_COM5 = 5,
};

#ifdef __cplusplus
extern "C" {
#endif

/* Create a new serial session
 * args:
 *    port - com port number (ie, 2 for COM2)
 *    baud - baud rate (ie, 96000)
 * return:
 *    handle to new session
 *    NULL on failure
 */
serial_t *serial_open(
  uint32_t port,
  uint32_t baud);

/* Close an existing serial session
 * args:
 *    serial - handle to existing serial session
 */
void serial_close(
  serial_t *serial);

/* Send serial data
 * args:
 *    serial - handle to existing serial session
 *    data   - pointer to source data
 *    nbytes - number of bytes to send
 * return:
 *    number of bytes sent
 *    0 on failure
 */
uint32_t serial_send(
  serial_t *serial,
  const void *data,
  size_t nbytes);

/* Read data from serial session
 * args:
 *    serial - handle to existing serial session
 *    dst    - buffer to receive read data
 *    nbytes - number of bytes to read
 * return:
 *    number of bytes read
 *    0 on failure
 */
uint32_t serial_read(
  serial_t *serial,
  void *dst,
  size_t nbytes);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FILE_SERIAL_H
