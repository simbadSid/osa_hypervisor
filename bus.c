/*
Copyright (C) Pr. Olivier Gruber.

This code is part of a suite of educational software,
it is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This code is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Classpath; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301 USA.

This is the emulation of the bus subsystem. It emulates not only
the bus itself but also all the devices whose controllers are connected
on the bus.

*/

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "asm.h"
#include "parser.h"
#include <time.h>
#include <fcntl.h>
#include "bus.h"
#include "core.h"

extern uint8_t *memory; // physical memory array, allocate globally
extern struct core* core; // registers and all
extern struct bus* bus;

/**
 * This is part of emulating either a serial line or a network card.
 * It provides the ability to connect through a Unix socket domain
 * via a file. This is called from the client side of the unix socket
 * domain in order to establish a connection.
 */
int unix_socket_connect(char * file) {
  struct sockaddr_un addr;
  size_t addr_length;
  int sock;

  sock = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("Socket create failed:");
    shutdown(sock,SHUT_RDWR);
    return -1;
  }

  addr.sun_family = AF_UNIX;
  addr_length = sizeof(addr.sun_family) + sprintf(addr.sun_path, "%s", file);
  if (connect(sock, (struct sockaddr *) &addr, addr_length) < 0) {
    // perror("Socket connect failed:");
    shutdown(sock,SHUT_RDWR);
    return -1;
  }

  return sock;
}

/**
 * This is part of emulating either a serial line or a network card.
 * It provides the accept incoming connections.
 * In normal operation, this is blocking until a connection is made.
 */
int unix_socket_accept(int ssock) {
  int sock;
  struct sockaddr_un address;
  socklen_t length = sizeof(address);

  sock = accept(ssock, (struct sockaddr *) &address, &length);
  if (sock == -1 && errno != EAGAIN) {
    perror("unix_socket_accept");
    exit(-1);
  }
  return sock;
}

/**
 * This is part of emulating either a serial line or a network card.
 * It binds a unix socket domain to the given file.
 */
int unix_socket_bind(char * path) {
  int sock;

  printf("unix socket: creating server socket on %s\n", path);
  if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
    perror("socket() failed");
    exit(666);
  }
  printf("unix socket: socket created : %d\n", sock);

  struct sockaddr_un address;
  unlink(path);
  address.sun_family = AF_UNIX;
  int address_length = sizeof(address.sun_family) +
      sprintf(address.sun_path,"%s",path);

  printf("unix socket: binding %d\n", sock);
  if (bind(sock, (struct sockaddr *) &address, address_length) < 0) {
    perror("bind() failed");
    exit(666);
  }

  if (listen(sock, 5) < 0) {
    perror("listen() failed");
    exit(666);
  }

  return sock;
}

/**
 * This is the actual wrapper of a unix socket domain to emulate
 * a serial line. It waits until a connection is established.
 * Look at the sockcon.c code to see the client side.
 */
void bus_serial_accept() {
  bus->serial.sock= -1;
  bus->serial.ssock= -1;

  bus->serial.ssock = unix_socket_bind(bus->serial.name);
  // fcntl(server, F_SETFL, O_NONBLOCK); // for non-blocking accept
  if (bus->serial.ssock<0) {
    printf("Could not bind to socket=%s\n", bus->serial.name);
    exit(-1);
  }
  printf("Serial Line Emulation:\n");
  printf("  Please start the sockcon program, like this:\n");
  printf("    $ ./sockcon -s:%s \n",bus->serial.name);

  bus->serial.sock = unix_socket_accept(bus->serial.ssock);
  if(bus->serial.sock == -1) {
    printf("Accept failed on socket=%s\n", bus->serial.name);
    exit(-1);
  }
  printf("-> Serial line attached ...\n");
  fcntl(bus->serial.sock, F_SETFL, O_NONBLOCK);
}

/**
 * This is the initialization of the bus, including the
 * initialization of all the devices whose controllers are
 * attached on the bus.
 */
void bus_init() {
  strcpy(bus->serial.name,".serial");
  bus->serial.status |= TX_AVAILABLE;

  // You must turn this back on, so that the emulation accept
  // a connection from the sockcon program...
#if 0
  bus_serial_accept();
#endif
}

/**
 * This is the emulation of devices.
 * It is called after every instruction fetched, decoded, and
 * executed by the core.
 */
void bus_emul_devices(void) {
}

/*
 * This is a load operation issued from the CPU on the bus.
 */
uint32_t bus_load(uint32_t addr) {

  // check the validity of the address.
  if (addr >= SIZE_MEM_ARRAY) {
    trap(TRAP_MEMORY_ERROR,addr,0);
    return 0xFFFFFFFF;
  }
  // just a read from physical memory
  return *((uint32_t*) (memory + addr));
}


/*
 * This is a store operation issued from the CPU on the bus.
 */
void bus_store(uint32_t addr, uint32_t value) {

  // check the validity of the address.
  if (addr >= SIZE_MEM_ARRAY) {
    trap(TRAP_MEMORY_ERROR,addr,0);
    return;
  }
  // just a write to physical memory
  *((uint32_t*) (memory + addr)) = value;
  return;
}
