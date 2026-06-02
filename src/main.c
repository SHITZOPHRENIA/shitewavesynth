// TODO: Add SDL keyboard input




#include <SDL3/SDL_video.h>
#include <stdio.h>
#include <math.h>
#include <portaudio.h>
#include <SDL3/SDL.h>


#define NUM_SECONDS         (.5)
#define SAMPLE_RATE         (44100)
#define FRAME_PER_BUFFER    (64)

#ifndef M_PI
#define M_PI                (3.14159265)
#endif

#define SCR_W (120)
#define SCR_H (100)


#define TABLE_SIZE          (200)
typedef struct {
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
    char message[20];

} paData;

// own Portaudio error function
// called if needed.
PaError oErr_PaError(PaError err) {
    Pa_Terminate();
    fprintf( stderr, "An error occured with portaudio\n" );
    fprintf( stderr, "Error number: %d\n", err);
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}

// own SDL3 error function
// called if needed.
void oErr_PrintSDLError(const char* errMsg) {
    fprintf(stderr, "%sSDL_Error: %s\n", errMsg, SDL_GetError());
}

static int paCallBack( const void *inputBuffer, void *outputBuffer,
                       unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo* timeInfo,
                       PaStreamCallbackFlags statusFlags,
                       void *userData )
{
    paData *data = (paData*)userData;
    float *out = (float*)outputBuffer;
    unsigned long i;

    (void) timeInfo;
    (void) statusFlags;
    (void) inputBuffer;

    for (i=0; i<framesPerBuffer; i++) {
        *out++ = data->sine[data->left_phase];
        *out++ = data->sine[data->right_phase];
        data->left_phase += 1;
        if (data->left_phase >= TABLE_SIZE) data->left_phase -= TABLE_SIZE;
        data->right_phase += 3;
        if (data->right_phase >= TABLE_SIZE) data->right_phase -= TABLE_SIZE;

    }

    return paContinue;

}

static void StreamFinished( void* userData ) {
    paData *data = (paData *) userData;
    printf( "Stream Completed: %s\n", data->message );

}

paData genSineData ( float pitch ) {
    paData data;
    int i;
    // init sine wave table
    for ( i=0; i<TABLE_SIZE; ++i ) {
        data.sine[i] = (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI  * pitch * 2.);

    }
    data.left_phase = data.right_phase = 0;

    return data;
}


PaError playNote( paData data, PaStream *stream, PaStreamParameters *oParams ) {

    PaError paErr;


    paErr = Pa_StartStream( stream );
    if ( paErr != paNoError ) return oErr_PaError(paErr);

    printf( "Play for %f seconds.\n", NUM_SECONDS );
    Pa_Sleep( NUM_SECONDS * 1000 );

    paErr = Pa_StopStream( stream );
    if ( paErr != paNoError ) return oErr_PaError(paErr);

    paErr = Pa_CloseStream( stream );
    if ( paErr != paNoError ) return oErr_PaError(paErr);

    printf("Note finished!\n");

    return paErr;

}

int main(void) {
    PaStream *stream;
    PaStreamParameters outputParameters;

    PaError paErr;


    // init Portaudio
    paErr = Pa_Initialize();
    if ( paErr != paNoError ) return oErr_PaError(paErr);

    if (outputParameters.device == paNoDevice) {
        fprintf( stderr, "Error: No Default Audio Device\n" );
        return oErr_PaError(paErr);
    }
    outputParameters.device = Pa_GetDefaultOutputDevice(); // default output device

    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    if ( paErr != paNoError ) return oErr_PaError(paErr);

    // init SDL3

    if( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
        oErr_PrintSDLError("Couldn't init SDL!\n");
        // return -1;

    }

    SDL_Window *window;
    window = SDL_CreateWindow("Piano Program", SCR_H, SCR_H, SDL_WINDOW_INPUT_FOCUS);
    if( window == NULL ) {
        oErr_PrintSDLError("Couldn't Create SDL Window!\n");
        // return -1;
    }

    SDL_Event event;

    // MAIN LOOP
    int running = -1;
    while (running != 0) {

        float pitch = 0;
        int i;

        paData data = genSineData(pitch);

        paErr = Pa_OpenStream(  &stream,
                                NULL, // no input
                                &outputParameters,
                                SAMPLE_RATE,
                                FRAME_PER_BUFFER,
                                paClipOff,
                                paCallBack,
                                &data );

        paErr = Pa_SetStreamFinishedCallback( stream, &StreamFinished );
        if ( paErr != paNoError ) return oErr_PaError(paErr);


        sprintf( data.message, "No Message" );

        for(i=0; i<13; i++) {
            paErr = Pa_OpenStream(&stream,
                                NULL, // no input
                                &outputParameters,
                                SAMPLE_RATE,
                                FRAME_PER_BUFFER,
                                paClipOff,
                                paCallBack,
                                &data );
            data = genSineData(pitch);
            if (pitch >= 12) pitch = 0;
            pitch += 1;
            playNote(data, stream, &outputParameters);

        }
    }


    Pa_Terminate();

    return 0;

}


