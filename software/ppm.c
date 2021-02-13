/*
 *  PPM to TX
 *  Copyright (C) 2010  Tomas 'ZeXx86' Jedrzejek (zexx86@zexos.org)
 *  Copyright (C) 2011  Tomas 'ZeXx86' Jedrzejek (zexx86@zexos.org)
 *  Copyright (C) 2017  Jürgen Diez (jdiez@web.de)
 *  Copyright (C) 2018  Tomas 'ZeXx86' Jedrzejek (tomasj@spirit-system.com)
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

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#ifndef __WIN32__
#include <portaudio.h>
#else
#include "lib/portaudio.h"
#endif
#include "sys.h"

#define SAMPLE_RATE       192000
#define FRAMES_PER_BUFFER 0
#define NUM_CHANNELS      1

#define NUM_TX_CHANNELS   6

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

typedef float SAMPLE;

typedef void PortAudioStream;
PortAudioStream *stream;

/* can be used for more channels */
static float channels[NUM_TX_CHANNELS];
static float channels1[NUM_TX_CHANNELS];
int c[NUM_TX_CHANNELS];

int app_exit;


static int callback_audio (const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo *timeInfo,
                           PaStreamCallbackFlags b, void *userData)
{
	static SAMPLE v_min = FLT_MAX;
	static SAMPLE v_max = -FLT_MAX;
	static SAMPLE v_th;
	static SAMPLE v_0 = 0;
	SAMPLE v_1;
	static PaTime pulseStartTime1 = 0;
  static PaTime pulseStartTime2 = 0;
	static int channel1 = -1;
  static int channel2 = -1;
	static unsigned int sampleNum1 = 0;
  static unsigned int sampleNum2 = 0;
	unsigned int i;

	SAMPLE *samplePtr = (SAMPLE*)inputBuffer;
	for (i = 0;
			i < framesPerBuffer;
			++i, ++sampleNum1, ++sampleNum2, samplePtr += NUM_CHANNELS, v_0 = v_1
  ) {
		v_1 = -*samplePtr;

		// Calculate the threshold between the two extrema
		v_min = MIN(v_min, v_1);
		v_max = MAX(v_max, v_1);
		v_th = (v_max + v_min) / 2.;

    short pos_slope = (v_0 <= v_th) && (v_1 > v_th);
    short neg_slope = (v_0 >= v_th) && (v_1 < v_th);

    if (pos_slope || neg_slope) {
      // Linear interpolation when hitting the threshold
      PaTime triggerOffset = ((v_th - v_1) / (v_1 - v_0)) / SAMPLE_RATE;

  		// Trigger pulse measurement on a positive slope
  		if (pos_slope) {
        PaTime triggerTime = ((float)sampleNum1 / SAMPLE_RATE) - triggerOffset;
  			PaTime pulseLength = triggerTime - pulseStartTime1;
  			pulseStartTime1 = triggerTime;

        // If the pulse is longer than 2ms it is considered a start pulse
  			if (pulseLength > 0.0021) {
  				channel1 = 0;
  				sampleNum1 = 0;
  				pulseStartTime1 = triggerOffset;
  			} else if (channel1 >= 0) {
  				// Store the pulse length in ms in the channel.
  				// According to the spec, the pulse length ranges from 1..2ms
  				channels1[channel1] = (pulseLength * 1000) - 1.5;
  				channel1++;
  				// Prevent channel overflow and ignore exceeding channels
  				if (channel1 >= NUM_TX_CHANNELS){
  					channel1 = -1;
  				}
  			}
  		}

      // Trigger second measurement of the same pulse on a negative slope
  		if (neg_slope) {
        PaTime triggerTime = ((float)sampleNum2 / SAMPLE_RATE) - triggerOffset;
        PaTime pulseLength = triggerTime - pulseStartTime2;
  			pulseStartTime2 = triggerTime;

        // If the pulse is longer than 2ms it is considered a start pulse
  			if (pulseLength > 0.0021) {
  				channel2 = 0;
  				sampleNum2 = 0;
  				pulseStartTime2 = triggerOffset;
  			} else if (channel2 >= 0) {
  				// Store the pulse length in ms in the channel.
  				// According to the spec, the pulse length ranges from 1..2ms
  				channels[channel2] = (channels1[channel2] + (pulseLength * 1000) - 1.5) / 2.0;
  				channel2++;
  				// Prevent channel overflow and ignore exceeding channels
  				if (channel2 >= NUM_TX_CHANNELS){
  					channel2 = -1;
  				}
  			}
  		}
    }

	}

	return 0;
}

int list_audio ()
{
	if (Pa_Initialize () != paNoError)
		return -1;

	const PaDeviceInfo *pdi;
	return Pa_GetDeviceCount ();
}

char *get_audio_name (int id)
{
	const PaDeviceInfo *pdi = Pa_GetDeviceInfo (id);

	if (pdi->maxInputChannels >= NUM_CHANNELS)
		return (char *) pdi->name;

	return 0;
}

int init_audio (int dev)
{
	if (Pa_Initialize () != paNoError)
		return -1;

	const PaDeviceInfo *pdi;
	int num_devs;

	num_devs = Pa_GetDeviceCount();

	int ret = -1;

	int i;
	for (i = 0; i < num_devs; i ++) {
		pdi = Pa_GetDeviceInfo (i);

		/* we've found selected device */
		if (i == dev) {
			ret = 0;

			if (pdi->maxInputChannels >= NUM_CHANNELS)
				printf("DeviceID (%d) name: %s.\n", i, pdi->name);
		}
	}

	return ret;
}

int close_audio ()
{
	Pa_Terminate();

	return 0;
}

int setup_audio (int dev)
{
	PaError err;
	PaStreamParameters inStreamParm;

	const PaDeviceInfo *pdi = Pa_GetDeviceInfo (dev);

	inStreamParm.device = dev;
	inStreamParm.channelCount = NUM_CHANNELS;
	inStreamParm.hostApiSpecificStreamInfo = NULL;
	inStreamParm.sampleFormat = paFloat32;
	inStreamParm.suggestedLatency = pdi->defaultLowInputLatency;

	/* Record some audio. -------------------------------------------- */
	err = Pa_OpenStream(&stream,
			    &inStreamParm,
			    NULL,
			    SAMPLE_RATE,
			    FRAMES_PER_BUFFER,
			    0,
			    callback_audio,
					NULL);

	if (err != paNoError)
		goto error;

	err = Pa_StartStream (stream);

	if (err != paNoError)
		goto error;

	return 0;

error:
	close_audio ();

	fprintf (stderr, "An error occured while using the portaudio stream in audio_rc_open\n");
	fprintf (stderr, "Error number: %d\n", err);
	fprintf (stderr, "Error message: %s\n", Pa_GetErrorText (err));

	return -1;
}

void ppm_decode (int fd, int mix)
{
	printf ("> PPM decoding is running at %dHz sample rate and %d frames per buffer on %d channel(s)\n", SAMPLE_RATE, FRAMES_PER_BUFFER, NUM_CHANNELS);

	int i;
	for (i = 0; i < NUM_TX_CHANNELS; ++i) {
		channels[i] = 0;
	}

	while (!app_exit) {
		// Reduce the CPU load by suspending the task
		Pa_Sleep(1);

		if (mix == 1) {
/*
			CCPM 120° - decode
			Pitch = (Ch1+Ch2+Ch6)/3
			Elevator = Pitch – Ch6
			Aileron = -Pitch + Ch1 – 0.5*Elevator
*/
			float pitch = (channels[0] + channels[1] - channels[5]) / 3;

			c[5] = (int) (pitch * 1024);
			c[1] = (int) ((channels[1] - pitch) * 1024);
			c[0] = (int) ((channels[0] - pitch + 0.5*  (channels[1] - pitch)) * 1800);
			c[2] = (int) (channels[2] * 1024);
			c[3] = (int) (channels[3] * 1280);
			c[4] = (int) (channels[4] * 996) - 8;
		} else if (mix == 2) {
			float pitch = (channels[1] + channels[2] - channels[5]) / 3;

			c[5] = (int) (pitch * 1024);
			c[1] = (int) ((channels[2] - pitch) * 1024);
			c[0] = (int) ((pitch - channels[1] - 0.5 * (channels[2] - pitch)) * 1800);
			c[2] = (int) (channels[0] * 1024);
			c[3] = (int) (channels[3] * 1280);
			c[4] = (int) (channels[4] * 996) - 8;
		} else {
			c[0] = (int) (channels[0] * 1024);
			c[1] = (int) (channels[1] * 1024);
			c[2] = (int) (channels[2] * 1024);
			c[3] = (int) (channels[3] * 1024);
			c[4] = (int) (channels[4] * 1024);
			c[5] = (int) (channels[5] * 1024);
		}

		for (i = 6; i < NUM_TX_CHANNELS; ++i) {
			c[i] = (int) (channels[i] * 512);
		}

		if (device_write (fd) == -1)
			break;
#ifdef DEBUG
		printf ("ch1: %d | ch2: %d | ch3: %d | ch4: %d | ch5: %d | ch6: %d\n", c[0], c[1], c[2], c[3], c[4], c[5]);
#endif
	}
}
