#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <winioctl.h>
#include "ppjioctl.h"

#define PORT		2600
#define DATA_LEN	12 * sizeof (int)

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

WORD wVersionRequested = MAKEWORD(2,0);
WSADATA data;
struct sockaddr_in sockName;
struct sockaddr_in clientInfo;
SOCKET Socket;
int addrlen;
	
int server_open (int port)
{
	if (WSAStartup (wVersionRequested, &data) != 0) {
		printf ("WINSOCK init problem!\n");        
		return -1;
	}    

	if ((Socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		printf ("Socket() problem !\n");
		WSACleanup();
		return -1;
	}

	sockName.sin_family = AF_INET;
	sockName.sin_port = htons (port);
	sockName.sin_addr.s_addr = INADDR_ANY;
	
	if (bind (Socket, (struct sockaddr *) &sockName, sizeof(sockName)) == SOCKET_ERROR) {
		printf ("Listen port is already used !\n");
		WSACleanup();
		return -1;
	}
	
	if (listen (Socket, 10) == SOCKET_ERROR) {
		printf ("Listen () error\n");
		WSACleanup();
		return -1;
	}
	
	return 0; 
}

int server_close ()
{
	closesocket (Socket);
	WSACleanup ();
	
	printf ("> server closed\n");
	
	return 0;
}

int server_recv (SOCKET fd, char *t, unsigned len)
{
       return recv (fd, t, len, 0);
}

int main (int argc, char **argv)
{
	HANDLE		h;
	JOYSTICK_STATE	JoyState;

	DWORD		RetSize;
	DWORD		rc;

	long		*Analog;
	char		*Digital;

	char		*DevName;

	DevName = "\\\\.\\PPJoyIOCTL1";
	
	if (argc == 2)
		DevName= argv[1];

	/* Open a handle to the control device for the first virtual joystick. */
	/* Virtual joystick devices are names PPJoyIOCTL1 to PPJoyIOCTL16. */
	h = CreateFile (DevName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	/* Make sure we could open the device! */
	if (h == INVALID_HANDLE_VALUE) {
		printf ("CreateFile failed with error code %d trying to open %s device\n",GetLastError(),DevName);
		return 1;
	}

	/* Initialise the IOCTL data structure */
	JoyState.Signature = JOYSTICK_STATE_V1;
	JoyState.NumAnalog = NUM_ANALOG;	/* Number of analog values */
	Analog = JoyState.Analog;		/* Keep a pointer to the analog array for easy updating */
	JoyState.NumDigital = NUM_DIGITAL;	/* Number of digital values */
	Digital = JoyState.Digital;		/* Digital array */

	printf ("PPJoy virtual joystick Server for TXPPM software -- controlling virtual joystick 1.\n\n");

	//memset (Digital,0,sizeof(JoyState.Digital));
	Analog[0] = Analog[1] = Analog[2] = Analog[3] = (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
	
	if (server_open (PORT) == -1) {
	      printf ("Failed to create server on port %d\n", PORT);
	      return -1;
	}
	
	printf ("> Server is listening on port %d\n", PORT);
	
	char buf[DATA_LEN + 1];
	
	SOCKET client;
getclient:
	addrlen = sizeof (clientInfo);
	client = accept (Socket, (struct sockaddr *) &clientInfo, &addrlen);
  
	if (client == INVALID_SOCKET) {
		printf ("Invalid client socket ()\n");
		WSACleanup ();
		return -1;
	}   
	
	for (;;) {
		int r = server_recv (client, buf, DATA_LEN);
		
		if (r == 0) {
			printf ("> Client disconnected\n");
			goto getclient;
		} else if (r < 0)
			continue;

		if (r != DATA_LEN) {
			printf ("> Malformed packet received from client !\n");
			continue;
		}
		
		int *channels = (int *) &buf;
		
		Analog[0] = channels[0] + (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
		Analog[1] = channels[1] + (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
		Analog[2] = channels[2] + (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
		Analog[3] = channels[3] + (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
		Analog[4] = channels[4] + (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
		Analog[5] = channels[5] + (PPJOY_AXIS_MIN+PPJOY_AXIS_MAX)/2;
		
		//printf ("channels: %d %d %d\n", Analog[0], Analog[1], Analog[2]);
		
		/* Send request to PPJoy for processing. */
		/* Currently there is no Return Code from PPJoy, this may be added at a */
		/* later stage. So we pass a 0 byte output buffer.                      */
		if (!DeviceIoControl (h, IOCTL_PPORTJOY_SET_STATE, &JoyState, sizeof (JoyState), NULL, 0, &RetSize, NULL)) {
			rc = GetLastError ();

			if (rc == 2) {
				printf ("Underlying joystick device deleted. Exiting read loop\n");
				break;
			}

			printf ("DeviceIoControl error %d\n", rc);
		}
	}

	CloseHandle (h);
	return 0;
}
