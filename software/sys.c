/*
 *  PPM to TX
 *  Copyright (C) 2010  Tomas 'ZeXx86' Jedrzejek (zexx86@zexos.org)
 *  Copyright (C) 2011  Tomas 'ZeXx86' Jedrzejek (zexx86@zexos.org)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "ppm.h"
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#ifndef __WIN32__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <conio.h>
#include <windows.h>
#include <winioctl.h>
#include "ppjioctl.h"
#include "lib/pthread.h"
#endif

#ifndef __WIN32__  /** ********************* LINUX/UNIX ********************* **/
void term_signal (int z)
{
	app_exit = 1;
}

int init_sig (void)
{
	struct sigaction sv;  
	
	memset (&sv, 0, sizeof (struct sigaction));
	sv.sa_flags = 0;
	sigemptyset (&sv.sa_mask);
	
	sv.sa_handler = SIG_IGN;
	/* Don't want broken pipes to kill the hub.  */
	sigaction (SIGPIPE, &sv, NULL);
	
	/* ...or any defunct child processes.  */
	sigaction (SIGCHLD, &sv, NULL);
	
	sv.sa_handler = term_signal;
	
	/* Also, shut down properly.  */
	sigaction (SIGTERM, &sv, NULL);
	sigaction (SIGINT, &sv, NULL);

	return 1;
}

struct hostent *host;
struct sockaddr_in serverSock;
socklen_t addrlen;

int client_connect (char *address, int port)
{
	  int fd;
  
	  if ((host = gethostbyname (address)) == NULL) {
		  printf ("> Invalid address: %s\n", address);
		  return -1;
	  }

	  if ((fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		  printf ("> socket () error\n");
		  return -1;
	  }

	  serverSock.sin_family = AF_INET;
	  serverSock.sin_port = htons (port);
	  memcpy (&(serverSock.sin_addr), host->h_addr, host->h_length);
	  
	  if (connect (fd, (struct sockaddr *) &serverSock, sizeof (serverSock)) == -1)
		  return -1;
  
	  return fd;
}

int client_send (int fd, char *s, unsigned l)
{
	return send (fd, s, l, 0);
}

int client_recv (int fd, char *s, unsigned l)
{
	return recv (fd, s, l, 0);
}

int client_close (int fd)
{
	close (fd);
}

int device_open ()
{
	int fd = open ("/dev/tx", O_WRONLY);

	if (fd < 0) {
		printf ("ERROR -> /dev/tx device is missing !\nDo you forget to create it over mknod or you need permission for write into this device, try chmod ?\n");
		return -1;
	}
 
 	printf ("> /dev/tx\n");

	return fd;
}

int device_write (int fd)
{
	int r = write (fd, &c, sizeof (int) * 12);
	
	if (r != sizeof (int) * 12) {
		app_exit = 1;
		printf ("ERROR -> Data are incomatibile length !\n");
	}
	
	return 0;
}

int device_close (fd)
{
	close (fd);
}
#else /** ********************* WIN32 ********************* **/

#define	NUM_ANALOG	6			/* Number of analog values which we will provide */
#define	NUM_DIGITAL	0			/* Number of digital values which we will provide */

#pragma pack(push,1)				/* All fields in structure must be byte aligned. */
typedef struct {
	unsigned long	Signature;		/* Signature to identify packet to PPJoy IOCTL */
	char		NumAnalog;		/* Num of analog values we pass */
	long		Analog[NUM_ANALOG];	/* Analog values */
	char		NumDigital;		/* Num of digital values we pass */
	char		Digital[NUM_DIGITAL];	/* Digital values */
}	JOYSTICK_STATE;
#pragma pack(pop)

HANDLE		h;
JOYSTICK_STATE	JoyState;

DWORD		RetSize;
DWORD		rc;

long		*Analog;
char		*Digital;
char		*DevName;


WSADATA data;
struct hostent *host;
struct sockaddr_in serverSock;
typedef int socklen_t;
socklen_t addrlen;

int client_connect (char *address, int port)
{
	int fd;
	WORD wVersionRequested = MAKEWORD (2,0);
	
	if (WSAStartup (wVersionRequested, &data) != 0) {
		printf ("> WinSock initialization error\n");
		return -1;
	}
  
	if ((host = gethostbyname (address)) == NULL) {
		printf ("> Invalid address: %s\n", address);
		return -1;
	}

	if ((fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf ("> socket () error\n");
		return -1;
	}

	serverSock.sin_family = AF_INET;
	serverSock.sin_port = htons (port);
	memcpy (&(serverSock.sin_addr), host->h_addr, host->h_length);
	  
	if (connect (fd, (struct sockaddr *) &serverSock, sizeof (serverSock)) == -1)
		return -1;
  
	return fd;
}

int client_send (int fd, char *s, unsigned l)
{
	return send (fd, s, l, 0);
}

int client_recv (int fd, char *s, unsigned l)
{
	return recv (fd, s, l, 0);
}

int client_close (int fd)
{
	closesocket (fd);
	WSACleanup ();
}

int device_open ()
{
	DevName = "\\\\.\\PPJoyIOCTL1";

	/* Open a handle to the control device for the first virtual joystick. */
	/* Virtual joystick devices are names PPJoyIOCTL1 to PPJoyIOCTL16. */
	h = CreateFile (DevName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	/* Make sure we could open the device! */
	if (h == INVALID_HANDLE_VALUE) {
		printf ("CreateFile failed with error code %d trying to open %s device\n",GetLastError(),DevName);
		return -1;
	}

	/* Initialise the IOCTL data structure */
	JoyState.Signature = JOYSTICK_STATE_V1;
	JoyState.NumAnalog = NUM_ANALOG;	/* Number of analog values */
	Analog = JoyState.Analog;		/* Keep a pointer to the analog array for easy updating */
	JoyState.NumDigital = NUM_DIGITAL;	/* Number of digital values */
	Digital = JoyState.Digital;		/* Digital array */

	printf ("PPJoy virtual joystick Server for TXPPM software -- controlling virtual joystick 1.\n\n");

	Analog[0] = Analog[1] = Analog[2] = Analog[3] = (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
	
	return 0;
}

int device_write (int fd)
{
	Analog[0] = c[0] + (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
	Analog[1] = c[1] + (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
	Analog[2] = c[2] + (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
	Analog[3] = c[3] + (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
	Analog[4] = c[4] + (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
	Analog[5] = c[5] + (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
	
	if (!DeviceIoControl (h, IOCTL_PPORTJOY_SET_STATE, &JoyState, sizeof (JoyState), NULL, 0, &RetSize, NULL)) {
		rc = GetLastError ();

		if (rc == 2) {
			printf ("Underlying joystick device deleted. Exiting read loop\n");
			return -1;
		}

		printf ("DeviceIoControl error %d\n", rc);
	}
	
	return 0;
}

int device_close (fd)
{
	CloseHandle (h);
}
#endif

static void thread_check (void *ptr)
{
	if (!ptr)
		goto end;
  
	int fd = client_connect (SERVER_INFO, SERVER_INFO_PORT);
	
	if (fd < 0)
		goto end;
#ifndef __WIN32__
# ifdef __linux__
	char s = 0;
# else
	char s = 1;
# endif
#else
	char s = 2;
#endif	
	if (client_send (fd, &s, 1) < 0) {
		client_close (fd);
		goto end;
	}
	  
	char ver[8];
	int r = client_recv (fd, ver, 8);
	
	client_close (fd);

	if (r < 1)
		goto end;
	else
	    ver[r] = '\0';

	typedef void (*callback) (char *v); 
	callback f = (callback) ptr;

	if (strcmp (ver, VERSION))
		f (ver);
end:
	pthread_exit (0); /* exit */
}

void check_version (void *ptr)
{
	pthread_t thread;
	pthread_create (&thread, NULL, (void *) &thread_check, ptr);
}
