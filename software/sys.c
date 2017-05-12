/*
 *  PPM to TX
 *  Copyright (C) 2010  Tomas 'ZeXx86' Jedrzejek (zexx86@zexos.org)
 *  Copyright (C) 2011  Tomas 'ZeXx86' Jedrzejek (zexx86@zexos.org)
 *  Copyright (C) 2017  Jürgen Diez (jdiez@web.de)
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
#include <unistd.h>
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
		printf ("ERROR -> Data are incompatibile length !\n");
	}

	return 0;
}

int device_close (int fd)
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
