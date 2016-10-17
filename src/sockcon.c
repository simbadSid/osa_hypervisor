/*
 * sockcon.c
 *
 *  Created on: Oct 3, 2016
 *      Author: ogruber
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include <termios.h>

#include <getopt.h>

#include <linux/unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>






void usage()
{
	printf("./sockcons -s:name[,server] \n");
}

/**
 * This is part of emulating the remote side of a serial line
 * as a keyboard and a terminal, via a Unix socket domain.
 * It provides the ability to connect through a Unix socket domain
 * via a file. This is called from the client side of the unix socket
 * domain in order to establish a connection.
 */
int unix_socket_connect(char * file)
{
	struct sockaddr_un addr;
	size_t addr_length;
	int sock;

	sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror("Socket create failed:");
		shutdown(sock,SHUT_RDWR);
		return -1;
	}
	addr.sun_family = AF_UNIX;
	addr_length = sizeof(addr.sun_family) + sprintf(addr.sun_path, "%s", file);
	if (connect(sock, (struct sockaddr *) &addr, addr_length) < 0)
	{
		// perror("Socket connect failed:");
		shutdown(sock,SHUT_RDWR);
		return -1;
	}
	return sock;
}

/**
 * This is part of emulating the remote side of a serial line
 * as a keyboard and a terminal, via a Unix socket domain.
 * It provides the accept incoming connections.
 * In normal operation, this is blocking until a connection is made.
 */
int unix_socket_accept(int sock)
{
	int client;
	struct sockaddr_un address;
	socklen_t length = sizeof(address);

	client = accept(sock, (struct sockaddr *) &address, &length);
	if (client == -1 && errno != EAGAIN)
	{
		perror("unix_socket_accept");
	}
	return client;
}

/**
 * This is part of emulating the remote side of a serial line
 * as a keyboard and a terminal, via a Unix socket domain.
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
  int address_length = sizeof(address.sun_family) + sprintf(address.sun_path,"%s",
      path);

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

/*
 * This is part of emulating the remote side of a serial line
 * as a keyboard and a terminal, via a Unix socket domain.
 * This is the heart of the console emulation, client side.
 */
void console(int sock) {
  printf("Console attached ...\n");
  fcntl(sock, F_SETFL, O_NONBLOCK);

  for (;;) {
    char outbuf[1025];
    int nread = read(sock, outbuf, 1024);
    if (nread < 0) {
      if (errno != EAGAIN) {
        printf("Console detached ...\n");
        return;
      }
    } else if (nread ==0) {
      printf("Console detached ...\n");
      return;
    } else {
      outbuf[nread]='\0';
      printf("%s",outbuf);
    }
    int available;
    char inbuf[1024];
    if (-1 != ioctl(0, FIONREAD, &available)) {
      if (available>0) {
        if (available>1024)
          available = 1024; // avoid buffer overflow...
        read(0, inbuf, available);
        int count = 0;
        while (count<available) {
          count += write(sock,inbuf+count,available-count);
        }
      }
    }
    fflush(stdout);
    usleep(10);
  }
}

struct termios tp, save;

/**
 * This is our hook on the process exit.
 * We reset the terminal state to its original condition
 * so that your Linux terminal behaves normally when this
 * process will exit.
 */
void __exit(void)
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &save);
}

/**
 * To really emulate a serial line, we must play
 * around with the Linux terminal setting.
 * Indeed, by standard, Linux terminal are not
 * only buffering characters until we hit the return key
 * but the typed characters are local echoed.
 * To emulate a serial line, we must turn off both
 * the buffering and the local echo.
 */
void stdin_setup()
{
	if (tcgetattr(STDIN_FILENO, &tp) == -1)				// Retrieve current terminal settings
		exit(-1);

	save = tp;											// save it so that we can restore it on exit
	tp.c_lflag &= ~(ICANON | ECHO);						// turn off echo and buffering, so that each character
														// typed is sent immediately to our emulation.

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tp) == -1)
		exit(-1);

	atexit(__exit);										// now set the callback at exit time
														// so we can restore a normal setting before exiting.
}












int main(int argc, char **argv)
{
	stdin_setup();

	/*
	 * Let's start by parsing the arguments.
	 */
	int c;
	char *name;
	int servermode = 0;

	while ((c = getopt(argc, argv, "s:")) != EOF)
	{
		switch (c)
		{
			case 's':
			{
				optarg++;
				char *coma = strchr(optarg, ',');
				if (coma != NULL)
				{
					int length = coma - optarg;
					name = malloc(length + 1);
					strncpy(name, optarg, length);
					name[length]='\0';
					if (strncmp(coma + 1, "server", 6) == 0)
					{
						servermode = 1;
						printf("server socket name=[%s]\n", name);
					}
					else
						printf("client socket name=[%s]\n", name);
				}
				else
				{
					int length = strlen(optarg);
					name = malloc(length + 1);
					strcpy(name, optarg);
					printf("client socket name=[%s]\n", name);
				}
				break;
			}
			default:
				usage();
		}
	}
	/*
	 * Let's see if we are to act as a client, connect to a server socket
	 * or a server, waiting for a client to connect.
	 */
	int sock= -1;
	if (servermode)
	{
		int ssock = -1;
		ssock = unix_socket_bind(name);
		// fcntl(server, F_SETFL, O_NONBLOCK); // for non-blocking accept
		if (ssock<0)
		{
			printf("Could not bind to socket=%s\n", name);
			exit(-1);
		}
		for (;;)
		{
			printf("Accepting on socket name=%s\n", name);
			sock = unix_socket_accept(ssock);
			if(sock != -1)
			{
				console(sock);
			}
		}
	}

	else
	{
		struct sockaddr_un addr;
		size_t addr_length;
		int sock;

		sock = socket(PF_UNIX, SOCK_STREAM, 0);
		if (sock < 0)
		{
			perror("Socket create failed:");
			return -1;
		}
		for (;;)
		{
			addr.sun_family = AF_UNIX;
			strcpy(addr.sun_path, name);
			addr_length = sizeof(addr.sun_family) + sprintf(addr.sun_path, "%s", name);
			// addr_length = sizeof(addr.sun_family) + strlen(addr.sun_path);
			int res = connect(sock, (struct sockaddr *) &addr, addr_length);
			if (res >= 0)
			{
				printf("Connected...\n");
				console(sock);
				// we must shutdown the socket, otherwise, it does not reconnect
				// anymore when a new server is available... Argh !$*@$&%^@&*$^%
				shutdown(sock, SHUT_RDWR);
				sock = socket(PF_UNIX, SOCK_STREAM, 0);
				if (sock < 0)
				{
					perror("Socket create failed:");
					return -1;
				}
			}
			else if (res <0)
			{
				perror("Connect:");
			}
			usleep(100000);
		}
	}
}
