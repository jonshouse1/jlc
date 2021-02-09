// player_initialise_pulse
// Version 1.0
// Last Changed 23 Mar 2013
//
// Open ALSA and set parameters
// number_of_samples_per_write  -       Number of samples per write, in this case 512 samples per channel
// number_of_channels           -       Humber of channels,  1=mono (not supported) 2=stereo (interleaved)
// sample_rate                  -       Sample rate in Hz 44100 for CD audio for example
// blocking_write               -       Open sound system blocking or non-blocking, for our application blocking is best


void initialise_sound_system(int number_of_samples_per_write, int number_of_channels, int sample_rate, int blocking_write)
{
	char name[1024]; 
	int errorb;

    	//paattr.tlength = buflen * 2;                                                                // Playback only, target length of the buffer (was *2)
    	//paattr.minreq = buflen;
    	//paattr.fragsize = buflen;
    	//paattr.prebuf = buflen;                                                                 // How much data pulse holds before it starts playing (was *4)
    	//paattr.maxlength = buflen * 2;                                                              // playback pipeline N samples deep (was *4)

    	paattr.tlength = 512;                                                                // Playback only, target length of the buffer (was *2)
    	paattr.minreq = 512;
    	paattr.fragsize = 512;
    	paattr.prebuf = 1024;                                                                 // How much data pulse holds before it starts playing (was *4)
    	paattr.maxlength = 1024;                                                              // playback pipeline N samples deep (was *4)


//investigate fixing this one day
        static const pa_sample_spec ss_networkaudio = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2 };

	printf("%s: PULSE: initialise_sound_system() - Using pulse audio\n",PROGNAME);
	fflush(stdout);

	//s = pa_simple_new(NULL, name, PA_STREAM_PLAYBACK, NULL, "playback", &ss_networkaudio, NULL, &paattr, &errorb); 
	if (!(s = pa_simple_new(NULL, name, PA_STREAM_PLAYBACK, NULL, "playback", &ss_networkaudio, NULL, NULL, &errorb))) // specify attributes ourselves
	//if (s == NULL)
        {
                fprintf(stderr, "pa_simple_new() failed: %s\n", pa_strerror(errorb));
		exit(1);
        }
        fflush(stderr);


	//if (!(s = pa_simple_new(NULL, name, PA_STREAM_PLAYBACK, NULL, "playback", &ss_networkaudio, NULL, NULL, &error))) // specify attributes ourselves
}
