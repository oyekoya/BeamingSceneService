/*
  Compile with:
  gcc -o play play.c $(pkg-config --cflags --libs sndfile portaudio-2.0)
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sndfile.h>
#include <portaudio.h>
#include "Getche.h"
#include "client.h"
#include <time.h>
#include <iostream>


/* #define SAMPLE_RATE  (17932) // Test failure to open with this value. */
#define SAMPLE_RATE  (44100)
#define NUM_CHANNELS    (2)
/* #define DITHER_FLAG     (paDitherOff)  */
#define DITHER_FLAG     (0) /**/
#define FRAMES_PER_BUFFER  (1024)

/* Select sample format. */
#if 1
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE  (0.0f)
#elif 0
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#elif 0
#define PA_SAMPLE_TYPE  paInt8
typedef char SAMPLE;
#define SAMPLE_SILENCE  (0)
#else
#define PA_SAMPLE_TYPE  paUInt8
typedef unsigned char SAMPLE;
#define SAMPLE_SILENCE  (128)

#endif

typedef struct
{
    int          frameIndex;  /* Index into sample array. */
    int          maxFrameIndex;
    float      *replaySamples;
}
paTestData;

#define WARN(...) do { fprintf(stderr, __VA_ARGS__); } while (0)
#define DIE(...) do { WARN(__VA_ARGS__); exit(EXIT_FAILURE); } while (0)
#define PA_ENSURE(...)									\
	do {										\
		if ((err = __VA_ARGS__) != paNoError) {					\
			WARN("PortAudio error %d: %s\n", err, Pa_GetErrorText(err));	\
			goto error;							\
		}									\
	} while (0)


/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int playCallback( void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         PaTimestamp outTime, void *userData )
{
    paTestData *data = (paTestData*)userData;
    float *rptr = &data->replaySamples[data->frameIndex * NUM_CHANNELS];
    float *wptr = (float*)outputBuffer;
    unsigned int i;
    int finished;
    unsigned int framesLeft = data->maxFrameIndex - data->frameIndex;
    (void) inputBuffer; /* Prevent unused variable warnings. */
    (void) outTime;
    int framesToPlay, samplesToPlay, samplesPerBuffer;

    if( framesLeft < framesPerBuffer )
    {
        framesToPlay = framesLeft;
        finished = 1;
    }
    else
    {
        framesToPlay = framesPerBuffer;
        finished = 0;
    }
    
    samplesToPlay = framesToPlay * NUM_CHANNELS;
    samplesPerBuffer = framesPerBuffer * NUM_CHANNELS;

    for( i=0; i<samplesToPlay; i++ )
    {
        *wptr++ = *rptr++;
    }
    for( ; i<framesPerBuffer; i++ )
    {
        *wptr++ = 0;  /* left */
        if( NUM_CHANNELS == 2 ) *wptr++ = 0;  /* right */
    }
    data->frameIndex += framesToPlay;

    return finished;
}


int main(int argc, char **argv)
{
    PortAudioStream *stream;
    PaError    err;
    paTestData data;
    int        totalFrames;
    int        numSamples;
    int        numBytes;

	SF_INFO sfi;
	SNDFILE *sf;
	char ch;
	sf_count_t nread;

	sfi.format	= (SF_FORMAT_WAV | SF_FORMAT_PCM_24) ;

	/* Open the input file */
	if (argc < 2)
		DIE("Syntax: %s <filename>\n", argv[0]);
	memset(&sfi, 0, sizeof(sfi));

	if (! (sf = sf_open (argv[1], SFM_READ, &sfi)))
	{	printf ("Error : could not open file : %s\n", argv[1]) ;
		puts (sf_strerror (NULL)) ;
		exit (1) ;
		}

 	if (! sf_format_check (&sfi))
	{	sf_close (sf) ;
		printf ("Invalid encoding\n") ;
		return 0;
		} ;

	numSamples = sfi.frames * sfi.channels;
	printf("samples = %i; %i\n",numSamples, sfi.samplerate);
    numBytes = numSamples * sizeof(float);
    data.replaySamples = (float *) malloc( numBytes );
    if( data.replaySamples == NULL )
    {
        printf("Could not allocate record array.\n");
        exit(1);
    }

	/* Initialise PortAudio and open stream for default output device */
	PA_ENSURE( Pa_Initialize() );

	/* Write file data to stream */
	nread = sf_read_float(sf, data.replaySamples, sfi.channels * sfi.frames);
	printf("no. of frames read = %i\n",nread);

    data.frameIndex = 0;
	data.maxFrameIndex = sfi.frames;
    /* Playback audio data.  -------------------------------------------- */
    data.frameIndex = 0;
    printf("Begin playback.\n"); fflush(stdout);
    err = Pa_OpenStream(
              &stream,
              paNoDevice,
              0,               /* NO input */
              PA_SAMPLE_TYPE,
              NULL,
              Pa_GetDefaultOutputDeviceID(),
              sfi.channels,
              PA_SAMPLE_TYPE,
              NULL,
              sfi.samplerate,
              FRAMES_PER_BUFFER,            /* frames per buffer */
              0,               /* number of buffers, if zero then use default minimum */
              paClipOff,       /* we won't output out of range samples so don't bother clipping them */
              playCallback,
              &data );
    if( err != paNoError ) goto error;

    if( stream )
    {
        err = Pa_StartStream( stream );
        if( err != paNoError ) goto error;
        printf("Waiting for playback to finish.\n"); fflush(stdout);

		bool quit = false;
		while( Pa_StreamActive( stream ) && (!quit) )
		{
			if (kbhit())
			{
				ch=getch();
				if (ch=='s' || ch=='S')
				{
					printf("Stopping playback ...\n");
					quit=true;
				}
			}
			Pa_Sleep(100);
		}

        err = Pa_CloseStream( stream );
        if( err != paNoError ) goto error;
        printf("Done.\n"); fflush(stdout);
    }
    free( data.replaySamples );

    Pa_Terminate();
	sf_close(sf);
    return 0;

error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
	sf_close(sf);
    return -1;
}

