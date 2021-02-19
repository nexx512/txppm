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

#define HIGH_INPUT_ERROR      0x01
#define LOW_SAMPLE_RATE_ERROR 0x02

#define MEASUREMENT_COMPLETE  0x01

typedef float SAMPLE;

typedef void PortAudioStream;
PortAudioStream *stream;

typedef struct CycleMeasurement {
  double pulseStartTime;
  unsigned int sampleNum;
  int pulse;
  float channels[NUM_TX_CHANNELS];
} CycleMeasurement;

/* can be used for more channels */
static float channels[NUM_TX_CHANNELS];
int c[NUM_TX_CHANNELS];

int app_exit;

void reset_measurement (CycleMeasurement *measurement)
{
  measurement->pulseStartTime = 0;
  measurement->sampleNum = 0;
  measurement->pulse = -1;
}

void print_error_status (PaStreamCallbackFlags statusFlags)
{
  if (statusFlags & paInputUnderflow) {
    fprintf (stderr, "Input underflor error.\n");
  }
  if (statusFlags & paInputOverflow) {
    fprintf (stderr, "Input overflow error.\n");
  }
  if (statusFlags & ~(paInputUnderflow | paInputOverflow)) {
    fprintf (stderr, "Sampling Error %ld\n", statusFlags);
  }
}

short calculate_pulse_length (SAMPLE v_th, SAMPLE v_1, SAMPLE v_diff, CycleMeasurement *measurement)
{
  // Linear interpolation of the time when hitting the threshold
  double triggerOffset = ((v_th - v_1) / v_diff) / SAMPLE_RATE;
  double triggerTime = ((double)measurement->sampleNum / SAMPLE_RATE) + triggerOffset;
  double pulseLength = triggerTime - measurement->pulseStartTime;
  measurement->pulseStartTime = triggerTime;

  short current_pulse = -1;

  // If the pulse is longer than 2ms it is considered a start pulse
  if (pulseLength > 0.0021) {
    measurement->pulse = 0;
    measurement->sampleNum = 0;
    measurement->pulseStartTime = triggerOffset;
  } else if (measurement->pulse >= 0) {
    // Store the pulse length in ms in the channel.
    // According to the spec, the pulse length ranges from 1..2ms
    measurement->channels[measurement->pulse] = (pulseLength * 1000) - 1.5;
    current_pulse = measurement->pulse;
    measurement->pulse++;
    // Prevent channel overflow and ignore exceeding channels
    if (measurement->pulse >= NUM_TX_CHANNELS) {
      measurement->pulse = -1;
    }
  }
  return current_pulse;
}

static int callback_audio (const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo *timeInfo,
                           PaStreamCallbackFlags statusFlags, void *userData)
{
	static SAMPLE v_min = FLT_MAX;
	static SAMPLE v_max = -FLT_MAX;
	SAMPLE v_th;
	static SAMPLE v_0 = 0;
	SAMPLE v_1;
  SAMPLE v_diff;
  static CycleMeasurement pos_slope_measurement = {0, 0, -1};
  static CycleMeasurement neg_slope_measurement = {0, 0, -1};
  static short error = 0;
	unsigned int i;

	SAMPLE *samplePtr = (SAMPLE*)inputBuffer;
	for (i = 0;
			i < framesPerBuffer;
			++i, ++pos_slope_measurement.sampleNum, ++neg_slope_measurement.sampleNum, samplePtr += NUM_CHANNELS, v_0 = v_1
  ) {
    if (statusFlags != 0) {
      print_error_status (statusFlags);
      fprintf (stderr, "Resetting measurements.\n");

      reset_measurement (&pos_slope_measurement);
      reset_measurement (&neg_slope_measurement);
      return 0;
    }

		v_1 = *samplePtr;

    // Calculate the threshold between the two extrema
		v_min = MIN(v_min, v_1);
		v_max = MAX(v_max, v_1);
		v_th = (v_max + v_min) / 2.;
    v_diff = (v_1 - v_0);

    if (!(error & HIGH_INPUT_ERROR) && (v_1 <= -0.9 || v_1 >= 0.9 )) {
      fprintf (stderr, "Input signal too high. Please reduce input gain and restart.");
      error |= HIGH_INPUT_ERROR;
    }

    if (!(error & LOW_SAMPLE_RATE_ERROR) && v_diff >= 1.9) {
      fprintf (stderr, "Sample rate too low for accurate meassurements. Try to increase the sample rate and restart.");
      error |= LOW_SAMPLE_RATE_ERROR;
    }

		// Trigger pulse measurement on a positive slope
		if ((v_0 <= v_th) && (v_1 > v_th)) {
      calculate_pulse_length (v_th, v_1, v_diff, &pos_slope_measurement);
		}

    // Trigger second measurement of the same pulse on a negative slope
    // The positive slope must be seen before the negative slope
		if ((v_0 >= v_th) && (v_1 < v_th)) {
      short pulse = calculate_pulse_length (v_th, v_1, v_diff, &neg_slope_measurement);
      if (pulse >= 0) {
        channels[pulse] = (pos_slope_measurement.channels[pulse] + neg_slope_measurement.channels[pulse]) / 2.0;
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
