#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include "uinput.h"

static int axes[] = { ABS_X, ABS_Y, ABS_Z, ABS_THROTTLE, ABS_RUDDER};
int num_axes = 5;

void setAxes(int fd, int* data, int size){
	for(int i = 0; i < size; ++i){
		struct input_event ev = {0};
		ev.type = EV_ABS;
		ev.code = axes[i];
		ev.value = data[i];
		write(fd, &ev, sizeof(ev));
	}
}

int createUinputDevice(){
	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	
	if(fd < 0) {
		fd = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK);
		
		if(fd < 0){
			printf("Couldn't open uinput.\n");
			exit(EXIT_FAILURE);
		}
	}

	int ret = ioctl(fd, UI_SET_EVBIT, EV_ABS);
	ioctl(fd, UI_SET_EVBIT, EV_KEY);


	struct uinput_user_dev uidev = {0};
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "TxPPM");

	for(int i = 0; i < num_axes; ++i){
		ioctl(fd, UI_SET_ABSBIT, axes[i]);

		uidev.absmin[axes[i]] = -256;
		uidev.absmax[axes[i]] = 256;
	}

	ret = write(fd, &uidev, sizeof(uidev));
	ret = ioctl(fd, UI_DEV_CREATE);

	return fd;
}

void closeInputDevice(int fd){
	ioctl(fd, UI_DEV_DESTROY);
}
	
