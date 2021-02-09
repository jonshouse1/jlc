// esp-yafdp-server_config.ch
//#include <ets_sys.h>
//#include <osapi.h>
//#include <os_type.h>


// User settings go here,  this describes the device itself
#define YAFDP_DEVICE_MANUFACTURER	"JONSHOUSE"
#define YAFDP_DEVICE_MODELNAME		"ESPTEST"
#define YAFDP_DEVICE_DESCRIPTION	"ESP-TEST"


// -1 entry indicates end of list
const char * yafdp_servicelist[][5] = {   {"-1","-1","-1","","" }
				      };
