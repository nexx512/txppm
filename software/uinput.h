#ifndef __UINPUT_H_
#define __UINPUT_H_


void setAxes(int fd, int* data, int size);
int createUinputDevice();

void closeInputDevice(int fd);

#endif
