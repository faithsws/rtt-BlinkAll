#ifdef PKG_BLINKALL_USING_GPIO_EXAMPLE
#include "blink_all.h"

static int blink_gpio()
{
	
	blink_regisger_gpio("led1",200,20,PKG_BLINKALL_USING_GPIO_PIN,PKG_BLINKALL_USING_GPIO_ACTIVE_LEVEL);

	blink_device_start("led1");
	return 0;
}
INIT_APP_EXPORT(config_led);

#endif
