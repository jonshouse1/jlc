
// Audio+ data.  This is audio plus some other useful information.  We could stick display data in here as well
// but the existing LED displays expect it on a port of its own, plus they might not want the audio.
struct audioplus
{
        char leadin[15];				// Some text to help checking packets are correct for us
	unsigned char short_frame_counter;		// counts roughly from 0 to 85, used to sync LEDs on slave players
	int16 mplayersamples[NUM_SAMPLES*2];		// 44.1Khz stereo interleaved data
	char sender_ip[32];				// Wide in case IPV6 - Yes, I know this exists in the IP stack but ....
	char md5hash[34];				// MD5HASH of the mp3 that is playing
	int loops_for_this_track;			// This is the frame of audio we are playing.  Together with the MD5 other processes can synchronise with the music
	unsigned int  RXPORT;				// The port the master is listening for keypresses on
	unsigned int  DISPLAYDATAPORT;			// what port we are sending UDP display data on
};



