#ifndef __BLINK_ALL_H__
#define __BLINK_ALL_H__
#include "stdint.h"
#include <rtthread.h>
#include <rtdevice.h>
struct blink_device_t;
struct blink_device_ops{
	void (*init)(struct blink_device_t*);
	void (*on)(struct blink_device_t*);
	void (*off)(struct blink_device_t*);
	void (*deinit)(struct blink_device_t*);
};

struct blink_device_t {
	char name[RT_NAME_MAX];
	rt_uint32_t interval;
	rt_uint32_t repeat_cnt;
	
	struct blink_device_ops *ops;
	void *user_data;
	rt_uint32_t remain_cnt; 
	
	rt_uint8_t state;
	rt_tick_t next_toggle_tick;
	rt_slist_t                 object_list;  

	struct rt_mutex lock;
};

struct blink_gpio_t{
	rt_base_t pin;
	int on_level;
};

#ifndef BLINK_UINT_MS
#define BLINK_UINT_MS	10
#endif

#define BLINK_ON_FOVEVER	UINT32_MAX
#define BLINK_REP_FOREVER	UINT32_MAX


rt_err_t blink_regisger(const char* name,rt_uint32_t interval,rt_uint32_t repeat_cnt,struct blink_device_ops * ops, void *user_data);
rt_err_t blink_regisger_gpio(const char* name,rt_uint32_t interval,rt_uint32_t repeat_cnt,rt_base_t pin, int on_level);
void blink_unregister(const char* name);

rt_err_t blink_device_start(const char* name);
rt_err_t blink_device_stop(const char* name);
rt_err_t blink_device_set(const char* name,rt_uint32_t interval,rt_uint32_t repeat_cnt);

#endif //__BLINK_ALL_H__