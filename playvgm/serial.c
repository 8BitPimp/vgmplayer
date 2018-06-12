/*                     .__       .__
 *   ______ ___________|__|____  |  |
 *  /  ___// __ \_  __ \  \__  \ |  |
 *  \___ \\  ___/|  | \/  |/ __ \|  |__
 * /____  >\___  >__|  |__(____  /____/
 *      \/     \/              \/
 */
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <assert.h>
#include <stdlib.h>

#include "serial.h"


struct serial_t {
  HANDLE handle;
};


static BOOL set_timeouts(
  HANDLE handle)
{
  COMMTIMEOUTS com_timeout;
  ZeroMemory(&com_timeout, sizeof(com_timeout));
  com_timeout.ReadIntervalTimeout = 3;
  com_timeout.ReadTotalTimeoutMultiplier = 3;
  com_timeout.ReadTotalTimeoutConstant = 2;
  com_timeout.WriteTotalTimeoutMultiplier = 3;
  com_timeout.WriteTotalTimeoutConstant = 2;
  return SetCommTimeouts(handle, &com_timeout);
}


serial_t *serial_open(
  uint32_t port,
  uint32_t baud_rate)
{
  // construct com port device name
  char dev_name[] = "COMX";
  dev_name[3] = '0' + port;
  if (port > 9) {
    return NULL;
  }
  // open handle to serial device
  HANDLE handle = CreateFileA(
    dev_name,
    GENERIC_READ | GENERIC_WRITE,
    0,
    NULL,
    OPEN_EXISTING,
    0,
    NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    goto on_error;
  }
  // query serial device control block
  DCB dbc;
  ZeroMemory(&dbc, sizeof(dbc));
  dbc.DCBlength = sizeof(dbc);
  if (GetCommState(handle, &dbc) == FALSE) {
    goto on_error;
  }
  // change baud rate
  if (dbc.BaudRate != baud_rate) {
    dbc.BaudRate = baud_rate;
    if (SetCommState(handle, &dbc) == FALSE) {
      goto on_error;
    }
  }
  // set com timeouts
  if (set_timeouts(handle) == FALSE) {
    goto on_error;
  }
  // wrap in serial object
  serial_t *serial = malloc(sizeof(serial_t));
  if (serial == NULL) {
    goto on_error;
  }
  serial->handle = handle;
  // success
  return serial;
  // error handler
on_error:
  if (handle != INVALID_HANDLE_VALUE)
    CloseHandle(handle);
  return NULL;
}

void serial_close(
  serial_t *serial)
{
  assert(serial);
  if (serial->handle != INVALID_HANDLE_VALUE) {
    CloseHandle(serial->handle);
  }
  free(serial);
}

uint32_t serial_send(
  serial_t *serial,
  const void *data,
  size_t nbytes)
{
  assert(serial && data && nbytes);
  DWORD nb_written = 0;
  if (WriteFile(
        serial->handle,
        data,
        nbytes,
        &nb_written,
        NULL) == FALSE) {
    return 0;
  }
  return nb_written;
}

uint32_t serial_read(
  serial_t *serial,
  void *dst,
  size_t nbytes)
{
  assert(serial && dst && nbytes);
  DWORD nb_read = 0;
  if (ReadFile(
        serial->handle,
        dst, nbytes,
        &nb_read,
        NULL) == FALSE) {
    return 0;
  }
  return nb_read;
}
