#include "pin_map.h"
#include "eagle_soc.h"

uint32_t pin_mux[GPIO_PIN_NUM] = {
	PERIPHS_IO_MUX_GPIO0_U,		// GPIO 0
	PERIPHS_IO_MUX_U0TXD_U, 	// 1
	PERIPHS_IO_MUX_GPIO2_U,  	// 2
	PERIPHS_IO_MUX_U0RXD_U,		// 3
	PERIPHS_IO_MUX_GPIO4_U, 	// 4
	PERIPHS_IO_MUX_GPIO5_U, 	// 5
	-1,				// 6
	-1, 				// 7
	-1, 				// 8
	PERIPHS_IO_MUX_SD_DATA2_U,	// 9
	PERIPHS_IO_MUX_SD_DATA3_U, 	// 10
	-1, 				// 11
	PERIPHS_IO_MUX_MTDI_U,		// 12
	PERIPHS_IO_MUX_MTCK_U,		// 13
	PERIPHS_IO_MUX_MTMS_U,		// 14
	PERIPHS_IO_MUX_MTDO_U,		// 15 
	PAD_XPD_DCDC_CONF
};


uint8_t pin_func[GPIO_PIN_NUM] = {
	FUNC_GPIO0, 
	FUNC_GPIO1, 
	FUNC_GPIO2, 
	FUNC_GPIO3, 
	FUNC_GPIO4, 
	FUNC_GPIO5, 
	-1, 
	-1, 
	-1, 
	FUNC_GPIO9, 
	FUNC_GPIO10, 
	-1, 
	FUNC_GPIO12, 
	FUNC_GPIO13, 
	FUNC_GPIO14,
	FUNC_GPIO15, 
	-1
};


bool is_valid_gpio_pin(uint8 gpiopin)
{
	if(gpiopin >= GPIO_PIN_NUM) return false;
	return (pin_func[gpiopin] != GPIO_PIN_FUNC_INVALID);
}
