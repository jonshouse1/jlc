// player_initialise_alsa
// Version 1.0
// Last Changed 23 Mar 2013
//
// Open ALSA and set parameters
// number_of_samples_per_write	- 	Number of samples per write, in this case 512 samples per channel
// number_of_channels		-	Humber of channels,  1=mono (not supported) 2=stereo (interleaved)
// sample_rate			-	Sample rate in Hz 44100 for CD audio for example
// blocking_write		-	Open sound system blocking or non-blocking, for our application blocking is best



	// playback buffer length
	//*********** buffer
	// Maths:
	// 86 frames of 512 samples a second  is 1000ms / 86 = 11.627906977ms per frame, call it 12
	// 80ms gives me 3528
	// 86ms gives me 3793
	// 96ms gives me 4234
	// 94ms gives me 4145
	// 93ms gives me 4101
	// 92ms gives me 4057




int alsa_bufsize=0;
void initialise_sound_system(int number_of_samples_per_write, int number_of_channels, int sample_rate, int blocking_write)
{
	if (blocking_write==TRUE)
	{
		printf("%s: ALSA: Trying to open alsa blocking\n",PROGNAME);
		rc = snd_pcm_open (&playback_handle, device, SND_PCM_STREAM_PLAYBACK, 0);
	}
	else
	{
		printf("%s: ALSA: Trying to open alsa non-blocking\n",PROGNAME);
		rc = snd_pcm_open (&playback_handle, device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	}

	if (rc < 0)	
	{
		fprintf (stderr, "%s: ALSA: initialise_sound_system() - cannot open audio device %s (%s)\n",PROGNAME, device, snd_strerror (rc));
		exit (1);
	}

	if ((rc = snd_pcm_hw_params_malloc (&hw_params)) < 0) 
	{
		fprintf (stderr, "%s: ALSA: initialise_sound_system() - cannot allocate hardware parameter structure (%s)\n",PROGNAME,snd_strerror (rc));
		exit (1);
	}
	if ((rc = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) 
	{
		fprintf (stderr, "%s: ALSA: initialise_sound_system() - cannot initialize hardware parameter structure (%s)\n",PROGNAME,snd_strerror (rc));
		exit (1);
	}

	printf("%s: ALSA: initialise_sound_system() - Set alsa number of channels %d\n",PROGNAME,number_of_channels);
	if ((rc = snd_pcm_hw_params_set_channels (playback_handle, hw_params, number_of_channels)) < 0) 
	{
		fprintf (stderr, "%s: ALSA: initialise_sound_system() - failed to set channel count to %d (%s) \n", PROGNAME, number_of_channels, snd_strerror (rc));
		exit (1);
	}

	if ((rc = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) 
	{
		fprintf (stderr, "%s: ALSA: initialise_sound_system() - alsa rejected stereo (%s)\n", PROGNAME, snd_strerror (rc));
		exit (1);
	}
	if ((rc = snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) 
	{
		fprintf (stderr, "%s: ALSA: initialise_sound_system() - cannot set sample format (%s)\n",PROGNAME, snd_strerror (rc));
		exit (1);
	}

	int requestedrate=sample_rate;
	//int exactrate=sample_rate;
//RECENT EDIT
	unsigned int exactrate=sample_rate;
	if ((rc = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &exactrate, 0)) < 0) 
	{
		fprintf (stderr, "%s: ALSA: initialise_sound_system() - cannot set sample rate (%s)\n",PROGNAME, snd_strerror (rc));
		exit (1);
	}
	if (exactrate!=requestedrate)
	{
		fprintf (stderr, "%s: ALSA: initialise_sound_system() - Something wrong, got sample rate %d expecting %d (%s)\n", PROGNAME, exactrate,requestedrate,snd_strerror (rc));
		exit(1);
	}




	// The lower this value the smaller the latency but the higher the probability of underruns on playback
	// bufchunks are the number of 512 sample buffers alsa is using
	// alsa buffers are set to this value,  start playing when N chunks are filled is set to this value -1
	// values of 1 are harmful and cause CPU hogging	
	// running non-root on a linux desktop a minimum value of 8 works,  the Pi itself can do better
	// values below 9 cause CPU hogging on PC

	// was 15
	int bufchunks = 20;


	//  wanted_buflen = 93 *  2 is a minimum, if you go for one frame then CPU usage goes very high and it doesn't work very well
	unsigned int buflen;
	unsigned int wanted_buflen = 12 * bufchunks;					// (12ms close enough) milliseconds * number of frames
	int dir=1;

	// Value is in useconds
	wanted_buflen=wanted_buflen * 1000;						// convert milliseconds to useconds
	buflen = wanted_buflen;
	if (snd_pcm_hw_params_set_buffer_time_near(playback_handle, hw_params, &buflen, &dir) < 0) 
	{
		fprintf(stderr,"%s: ALSA: initialise_sound_system() - Couldn't set buffer length",PROGNAME);
		exit(1);
	}
	if (buflen != wanted_buflen) 
	{
		fprintf(stderr,"%s: ALSA: initialise_sound_system() - Couldn't get wanted buffer size of %u, got %u instead\n",PROGNAME, wanted_buflen, buflen);
	}

        snd_pcm_uframes_t bufsize;
	if (snd_pcm_hw_params_get_buffer_size(hw_params, &bufsize) < 0) 
	{
		fprintf(stderr,"%s: ALSA: initialise_sound_system() - Couldn't get buffer size\n",PROGNAME);
		exit(1);
	}
	printf("%s: ALSA: initialise_sound_system() - Got buffer size (frames): %lu = %lu chunks(of %d samples) of audio\n",PROGNAME, bufsize,bufsize/NUM_SAMPLES,NUM_SAMPLES);
alsa_bufsize=bufsize;


	unsigned int wanted_period = 12 * 1000;						// 12ms as useconds 
	unsigned int actual_period = wanted_period;
	if (snd_pcm_hw_params_set_period_time_near(playback_handle, hw_params, &actual_period, &dir) < 0) 
	{
		printf("%s: ALSA: initialise_sound_system() - Couldn't set period length\n",PROGNAME);
		fflush(stdout);
	}
	if (actual_period != wanted_period) 
	{
		printf("%s: ALSA: initialise_sound_system() - Couldn't set wanted period time of %dus, got %dus instead\n",PROGNAME, wanted_period, actual_period);
		fflush(stdout);
	}

	snd_pcm_uframes_t period; 
	if (snd_pcm_hw_params_get_period_size(hw_params, &period, &dir) < 0) 
	{
		printf("%s: ALSA: initialise_sound_system() - Couldn't get period size",PROGNAME);
	}
	printf("%s: ALSA: initialise_sound_system() - got period_size %lu  dir=%d\n",PROGNAME,period,dir);



	if ((rc = snd_pcm_hw_params (playback_handle, hw_params)) < 0) 
	{
		fprintf (stderr, "%s: ALSA: initialise_sound_system() - cannot set parameters (%s)\n", PROGNAME,snd_strerror (rc));
		exit (1);
	}
	snd_pcm_hw_params_free (hw_params);










	// Prepare software params struct
	snd_pcm_sw_params_t *sw_params;
	snd_pcm_sw_params_malloc (&sw_params);

	// Get current parameters
	if (snd_pcm_sw_params_current(playback_handle, sw_params) < 0) 
	{
		fprintf(stderr,"%s: ALSA: initialise_sound_system() - Couldn't get current SW params\n",PROGNAME);
		fflush(stderr);
	}

	// How full the buffer must be before playback begins
	// value is frames (samples)
	// if this value is to low then you get constant underruns with broken audio, even one per playsamples() call if its bad
	// a value of NUM_SAMPLES * 4 seems to play 'in-sync' on the Pi
	//if (snd_pcm_sw_params_set_start_threshold(playback_handle, sw_params, NUM_SAMPLES * bufchunks-2 ) < 0) 
	if (snd_pcm_sw_params_set_start_threshold(playback_handle, sw_params, NUM_SAMPLES * bufchunks-1 ) < 0) 
	{
		fprintf(stderr,"%s: ALSA: initialise_sound_system() - Failed setting start threshold\n",PROGNAME);
		fflush(stderr);
	}

	// The the largest write guaranteed never to block
	// value is frames (samples)
	//if (snd_pcm_sw_params_set_avail_min(playback_handle, sw_params, NUM_SAMPLES) < 0) 
	if (snd_pcm_sw_params_set_avail_min(playback_handle, sw_params, NUM_SAMPLES*(bufchunks/2)) < 0) 
	{
		fprintf(stderr,"%s: ALSA: initialise_sound_system() - Failed setting min available buffer",PROGNAME);
		fflush(stderr);
	}

	// Apply settings
	if (snd_pcm_sw_params(playback_handle, sw_params) < 0) 
	{
		fprintf(stderr,"%s: ALSA: initialise_sound_system() - Failed wring sw_params",PROGNAME);
		fflush(stderr);
	}

	// And free struct again
	snd_pcm_sw_params_free(sw_params);



	if ((rc = snd_pcm_prepare (playback_handle)) < 0) 
	{
		fprintf (stderr, "%s: ALSA: cannot prepare audio interface for use (%s)\n",PROGNAME,  snd_strerror (rc));
		exit (1);
	}

	sprintf(playbacksystem,"ALSA lib:%s",snd_asoundlib_version());
	printf("%s: ALSA: initialise_sound_system() - using alsalib [%s]\n",PROGNAME,snd_asoundlib_version());
	fflush(stdout);
	fflush(stderr);
//exit(0);
}
