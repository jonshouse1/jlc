// DS18B20 ROM contents and temperature readings

#define MAX_TS	8												// Maximum number of temp sensors

// An array of these holds the data for all one wire connected sensors
struct tsensors
{
	uint8	active;												// True if this record is active
        uint8   rom[8];
        float   ftemp;
	float   pftemp;												// previous value
	char	atemp[7];											// Always SNN.XX\0  Sign,Hundreds,Tens.UnitUnit
} __attribute__((packed));

