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


#include <stdio.h>
#include <stdlib.h>
#include "lib/portaudio.h"
#include "sys.h"

#define SAMPLE_RATE		44100
#define NUM_SAMPLES		3000
#define NUM_CHANNELS		2

typedef float SAMPLE;

typedef struct {
	int     frameIndex;  /* Index into sample array. */
	int     frameSize;
	int     SamplesCounter;
	int     LastSamplesCounter;
	SAMPLE  *recordedSamples;
} paAudioData;

static paAudioData  AudioData;
static int          synchro_index = 0;

//#define PORTAUDIO 18

#if PORTAUDIO != 18
typedef void PortAudioStream;
#define PaStream PortAudioStream
#endif

PortAudioStream *stream;

/* can be used for more channels */
static float channels[12];
int c[12];

int app_exit;

int get_data (float *values, int *nvalues)
{
	float x, px, max, min, moy;
	int nval = AudioData.frameSize;
	int i, j, chanel = 0;
	float time, dt;
	float *sig = AudioData.recordedSamples;

	max = -100000;
	min = 100000;
	
	for (i = 0; i < nval; i ++) {
		x = sig[i];
		
		if (x > max)
			max = x;
		if (x < min)
			min = x;
	}
	
	moy = (max + min) / 2;

	x = 0;

	chanel = -1;

	time = 0;

	j = (AudioData.frameIndex + 1024) % AudioData.frameSize;

	for (i = 0; i < nval; i ++) {
		time ++;

		if (time > 100) {                   // Synchro detection
			if (chanel < 0)
				chanel = 0; //start
			else if (chanel > 0)
				break; //end
		}
		
		px = x;
		x = sig[j ++];

		if (j == AudioData.frameSize) {
			j = 0;
		}

		if ((x > moy) && (px < moy)) {
			dt = (moy - px) / (x - px);

			if (chanel >= 0) {
				if (chanel == 0 && i < (nval / 2)) {  
					synchro_index = j;  //for OSCILLO  synchro
				}

				if (chanel > 0) {
					values[chanel - 1] = (time + dt) / 44.1 - 1.5;
				}

				chanel ++;

				if (chanel > 10)
					break;
			}

			time = -dt;
		}
	}

	*nvalues = chanel - 1;

	return 1;
}

int get_data_audio (float *values)
{
	static int nvals = 0;

	static float vals[11];
	int i,n;
	
	/*
	* there is no need to update values more than 50 times a second
	* then lest check here every (SAMPLE_RATE/50) recorded samples
	*/

	Pa_Sleep (1); // not sure this is usefull

	n = AudioData.SamplesCounter;
	
	if (n - AudioData.LastSamplesCounter > SAMPLE_RATE / 50) {
		AudioData.LastSamplesCounter = n;		// lets update values
		
		if (get_data (vals, &n)) {
			for (i = 0; i < 11; i ++)
				values[i] = vals[i];

			nvals = n;
		}
	} else
	  /*
	  * LastSamplesCounter & SamplesCounter saturation management
	  */
		if (n < AudioData.LastSamplesCounter)
			AudioData.LastSamplesCounter = n;

	return nvals;
}


#if PORTAUDIO == 18
static int callback_audio (void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           PaTimestamp outTime, void *userData)
#else
static int callback_audio (const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo *outTime,
                           PaStreamCallbackFlags b, void *userData)
#endif
{
	paAudioData *data = (paAudioData *) userData;
	SAMPLE *rptr = (float *) inputBuffer;
	SAMPLE *wptr;
      
	unsigned long i;
	unsigned long framesLeft = data->frameSize - data->frameIndex;

	/* Prevent unused variable warnings. */
	outputBuffer = NULL;
	outTime = 0;

#if PORTAUDIO != 18
	b = 0;
#endif

	data->SamplesCounter += framesPerBuffer;

	wptr = &data->recordedSamples[data->frameIndex];

	if (framesPerBuffer <= framesLeft) {
		for (i = 0; i < framesPerBuffer; i ++) {
			*wptr ++ = *rptr ++;
			rptr ++;
		}  // assume NUM_CHANNELS = 2
	} else {
		for (i = 0; i < framesLeft; i ++) {
			*wptr ++ = *rptr ++;
			rptr ++;
		} // assume NUM_CHANNELS = 2

		wptr = &data->recordedSamples[0];

		for (i = 0; i < framesPerBuffer-framesLeft; i ++) {
			*wptr ++ = *rptr ++;
			rptr ++;
		}  // assume NUM_CHANNELS = 2
	}

	data->frameIndex = (data->frameIndex + framesPerBuffer) % data->frameSize;

	return 0;
}

int list_audio ()
{
	if (Pa_Initialize () != paNoError)
		return -1;
	  
	const PaDeviceInfo *pdi;
	int num_devs;
	
#if PORTAUDIO == 18
	num_devs = Pa_CountDevices ();
#else
	num_devs = Pa_GetDeviceCount ();
#endif	
	return num_devs;
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
	
#if PORTAUDIO == 18
	num_devs = Pa_CountDevices();
#else
	num_devs = Pa_GetDeviceCount();
#endif
	
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
#if PORTAUDIO != 18
	PaStreamParameters inStreamParm;
#endif
	PaError err;

	const PaDeviceInfo *pdi = Pa_GetDeviceInfo (dev);
  
	/*  configure device */
	AudioData.frameSize = NUM_SAMPLES; /* Record for a few samples. */
	AudioData.frameIndex = 0;
	AudioData.LastSamplesCounter = 0;
	AudioData.SamplesCounter = 0;

	AudioData.recordedSamples = (SAMPLE *) malloc (AudioData.frameSize * sizeof (SAMPLE));

	if (AudioData.recordedSamples == NULL) {
		printf ("ERROR -> Could not allocate record array\n");
		exit (1);
	}
	
	int i;
	for (i = 0; i < AudioData.frameSize; i ++)
		AudioData.recordedSamples[i] = 0;

#if PORTAUDIO != 18
	inStreamParm.device = dev;
	inStreamParm.channelCount = NUM_CHANNELS;
	inStreamParm.hostApiSpecificStreamInfo = NULL;
	inStreamParm.sampleFormat = paFloat32;
	inStreamParm.suggestedLatency = pdi->defaultHighInputLatency;
#endif

	/* Record some audio. -------------------------------------------- */
#if PORTAUDIO == 18
	err = Pa_OpenStream(&stream,
			    dev,
			    NUM_CHANNELS,  /* NUM_CHANNELS : stereo input */
			    paFloat32,
			    NULL,
			    paNoDevice,
			    0,
			    paFloat32,
			    NULL,
			    SAMPLE_RATE,
			    1024,          /* frames per buffer */
			    0,             /* number of buffers, if zero then use default minimum */
			    0, //paDitherOff,    /* flags */
			    callback_audio,
			    &AudioData );
#else
	err = Pa_OpenStream(&stream,
			    &inStreamParm,
			    NULL,
			    SAMPLE_RATE,
			    1024,
			    0,
			    callback_audio,
			    &AudioData);
#endif

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
	printf ("> PPM decoding is running\n");
	
	while (!app_exit) {
		get_data_audio ((float *) &channels);
		
		if (mix == 1) {
/*
			CCPM 120° - decode
			Pitch = (Ch1+Ch2+Ch6)/3
			Elevator = Pitch – Ch6
			Aileron = -Pitch + Ch1 – 0.5*Elevator
*/
			float pitch = (channels[0]+channels[1]-channels[5])/3;
			
			c[5] = (int) (pitch * 1024);
			c[1] = (int) ((-pitch + channels[1]) * 1024);
			c[0] = (int) ((-pitch + channels[0] + 0.5*(-pitch + channels[1])) * 1800);
			c[2] = (int) (channels[2] * 1024);
			c[3] = (int) (channels[3] * 1280);
			c[4] = (int) (channels[4] * 996) - 8;
		} if (mix == 2) {
			float pitch = (channels[1] + channels[2] - channels[5])/3;

			c[5] = (int) (pitch * 1024);
			c[1] = (int) ((channels[2] - pitch) * 1024);
			c[0] = (int) ((pitch - channels[1] - 0.5*(channels[2] - pitch)) * 1800);
			c[2] = (int) (channels[0] * 1024);
			c[3] = (int) (channels[3] * 1280);
			c[4] = (int) (channels[4] * 996) - 8;
		} else {
			c[0] = (int) (channels[0] * 512);
			c[1] = (int) (channels[1] * 512);
			c[2] = (int) (channels[2] * 512);
			c[3] = (int) (channels[3] * 512);
			c[4] = (int) (channels[4] * 512);
			c[5] = (int) (channels[5] * 512);
		}
		
		if (device_write (fd) == -1)
			break;
#ifdef DEBUG
		printf ("ch1: %d | ch2: %d | ch3: %d | ch4: %d | ch5: %d | ch6: %d\n", c[0], c[1], c[2], c[3], c[4], c[5]);
#endif
	}
}
