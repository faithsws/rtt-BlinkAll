#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "blink_all.h"


static rt_slist_t blink_devices = RT_SLIST_OBJECT_INIT(blink_devices);
static struct rt_mutex list_lock;
static struct blink_device_ops gpio_blink_device_ops;


rt_err_t blink_regisger(const char* name,rt_uint32_t interval,rt_uint32_t repeat_cnt,struct blink_device_ops * ops, void *user_data)
{
	rt_slist_t * l = RT_NULL;
	struct blink_device_t * bd = RT_NULL;
	
	/* 寻找同名设备， 相同则覆盖*/
		
	rt_slist_for_each(l,&blink_devices)
	{
		struct blink_device_t *bd_temp = rt_slist_entry(l,struct blink_device_t,object_list);
		
		if(strncmp(bd_temp->name,name,RT_NAME_MAX) == 0)
		{
			bd = bd_temp;
			break;
		}
	
	}
	rt_mutex_take(&list_lock,RT_WAITING_FOREVER);
	if(bd == RT_NULL)
	{
		bd = (struct blink_device_t*)rt_malloc(sizeof(struct blink_device_t));
		if(bd == RT_NULL)return -RT_ENOMEM;
		rt_slist_append(&blink_devices, &(bd->object_list));
		rt_strncpy(bd->name, name, RT_NAME_MAX);
	}
	bd->user_data = user_data;	
	bd->interval = interval;
	bd->repeat_cnt = repeat_cnt;
	bd->remain_cnt = 0;
	bd->next_toggle_tick = 0;
	bd->ops = ops;
	bd->state = 0;
	if(bd->ops && bd->ops->init)
	{
		bd->ops->init(bd);
	}	
	rt_mutex_init(&bd->lock,"bllock",RT_IPC_FLAG_FIFO);
	
	rt_mutex_release(&list_lock);
	return RT_EOK;
}

void blink_unregister(const char* name)
{
	rt_slist_t * l = RT_NULL;
	struct blink_device_t * bd = RT_NULL;
	for(l = (&blink_devices)->next; l != &blink_devices; l = l->next)							
	{
		struct blink_device_t *bd_temp = rt_slist_entry(l,struct blink_device_t,object_list);
		
		if(strncmp(bd_temp->name,name,RT_NAME_MAX) == 0)
		{
			bd = bd_temp;
			break;
		}
	
	}
	if(bd != RT_NULL)
	{
		if(bd->ops && bd->ops->deinit)
		{
			bd->ops->deinit(bd);
		}
		
		if(&gpio_blink_device_ops == bd->ops)
		{
			rt_free(bd->user_data);
		}
		
		rt_mutex_take(&list_lock,RT_WAITING_FOREVER);
		rt_mutex_detach(&bd->lock);
		rt_slist_remove(&blink_devices,&bd->object_list);
		rt_free(bd);
		bd = RT_NULL;
		rt_mutex_release(&list_lock);
	}
	
}

static void gpio_blink_init(struct blink_device_t* device)
{
	struct blink_gpio_t * gpio = (struct blink_gpio_t*) device->user_data;
	rt_pin_mode(gpio->pin, PIN_MODE_OUTPUT);
}
static void gpio_blink_on(struct blink_device_t* device)
{
	struct blink_gpio_t * gpio = (struct blink_gpio_t*) device->user_data;
	
	rt_pin_write(gpio->pin, gpio->on_level);
}
static void gpio_blink_off(struct blink_device_t* device)
{
	struct blink_gpio_t * gpio = (struct blink_gpio_t*) device->user_data;
	
	rt_pin_write(gpio->pin, !gpio->on_level);
}
static struct blink_device_ops gpio_blink_device_ops = {
	.init = gpio_blink_init,
	.on = gpio_blink_on,
	.off = gpio_blink_off
};

rt_err_t blink_regisger_gpio(const char* name,rt_uint32_t interval,rt_uint32_t repeat_cnt, rt_base_t pin, int on_level)
{
	rt_err_t result = RT_EOK;
	struct blink_gpio_t* gpio = (struct blink_gpio_t*)rt_malloc(sizeof(struct blink_gpio_t));
	if(gpio == RT_NULL)return -RT_ENOMEM;
	

	gpio->pin = pin;
	gpio->on_level = on_level;

	result = blink_regisger(name,interval,repeat_cnt,&gpio_blink_device_ops, gpio) ;
	if(result != RT_EOK)
	{
		rt_free(gpio);
	}
	
	return result;
}

rt_err_t blink_device_set(const char* name,rt_uint32_t interval,rt_uint32_t repeat_cnt)
{
	rt_slist_t * l = RT_NULL;

	rt_slist_for_each(l,&blink_devices)
	{
		struct blink_device_t *bd_temp = rt_slist_entry(l,struct blink_device_t,object_list);
	
		if(strncmp(bd_temp->name,name,RT_NAME_MAX) == 0)
		{
			rt_mutex_take(&bd_temp->lock,RT_WAITING_FOREVER);
			bd_temp->interval = interval;
			bd_temp->repeat_cnt = repeat_cnt;
			bd_temp->state = 0;
			bd_temp->remain_cnt = 0;
			bd_temp->next_toggle_tick = 0;
			if(bd_temp->ops && bd_temp->ops->off)
			{
				bd_temp->ops->off(bd_temp);
			}
			rt_mutex_release(&bd_temp->lock);
			return RT_EOK;
		}
	}
	
	return -RT_EINVAL;
}
rt_err_t blink_device_start(const char* name)
{
	rt_slist_t * l = RT_NULL;

	rt_slist_for_each(l,&blink_devices)
	{
		struct blink_device_t *bd_temp = rt_slist_entry(l,struct blink_device_t,object_list);
		
		if(strncmp(bd_temp->name,name,RT_NAME_MAX) == 0)
		{
			rt_mutex_take(&bd_temp->lock,RT_WAITING_FOREVER);
			bd_temp->remain_cnt = bd_temp->repeat_cnt;
			bd_temp->next_toggle_tick = rt_tick_get() + rt_tick_from_millisecond(bd_temp->interval);
			
			bd_temp->state = 1;
			
			if(bd_temp->ops && bd_temp->ops->on)
			{
				bd_temp->ops->on(bd_temp);
			}
			rt_mutex_release(&bd_temp->lock);
			return RT_EOK;
		}
	
	}
	return -RT_EINVAL;
}

rt_err_t blink_device_stop(const char* name)
{
	rt_slist_t * l = RT_NULL;

	rt_slist_for_each(l,&blink_devices)
	{
		struct blink_device_t *bd_temp = rt_slist_entry(l,struct blink_device_t,object_list);
		
		if(strncmp(bd_temp->name,name,RT_NAME_MAX) == 0)
		{
			rt_mutex_take(&bd_temp->lock,RT_WAITING_FOREVER);
			bd_temp->remain_cnt = 0;
			bd_temp->state = 0;
			if(bd_temp->ops && bd_temp->ops->off)
			{
				bd_temp->ops->off(bd_temp);
			}
			bd_temp->next_toggle_tick = 0;
			rt_mutex_release(&bd_temp->lock);
			return RT_EOK;
		}
	
	}
	return -RT_EINVAL;
}

static void blink_tmr(void *p)
{
	rt_slist_t * l = RT_NULL;

	while(1)
	{
		rt_tick_t now = rt_tick_get();
		rt_mutex_take(&list_lock,RT_WAITING_FOREVER);
		rt_slist_for_each(l,&blink_devices)
		{
			struct blink_device_t *bd_temp = rt_slist_entry(l,struct blink_device_t,object_list);
			rt_mutex_take(&bd_temp->lock,RT_WAITING_FOREVER);
			if(bd_temp->interval == BLINK_ON_FOVEVER)
			{
				if(bd_temp->next_toggle_tick != 0)
				{
					if(bd_temp->ops && bd_temp->ops->on)
					{
						bd_temp->ops->on(bd_temp);
					}
					bd_temp->state = 1;
				}else
				{
					if(bd_temp->ops && bd_temp->ops->off)
					{
						bd_temp->ops->off(bd_temp);
					}
					bd_temp->state = 0;
				}
			}
			else if(bd_temp->next_toggle_tick <= now)
			{

				if(bd_temp->remain_cnt > 0)
				{
					if(bd_temp->state)
					{
						if(bd_temp->ops && bd_temp->ops->off)
						{
							bd_temp->ops->off(bd_temp);
						}
						if(bd_temp->remain_cnt != BLINK_REP_FOREVER)
						{
							bd_temp->remain_cnt --;
						}
					}
					else
					{
						if(bd_temp->ops && bd_temp->ops->on)
						{
							bd_temp->ops->on(bd_temp);
						}
					}
					bd_temp->state = !bd_temp->state;
					
					bd_temp->next_toggle_tick = rt_tick_get() + rt_tick_from_millisecond(bd_temp->interval);
				}
				else
				{
					bd_temp->next_toggle_tick =  0;
				}

			}
			
			rt_mutex_release(&bd_temp->lock);

		}
		rt_mutex_release(&list_lock);
		
		rt_thread_mdelay(BLINK_UINT_MS);
	}
}
#define BLINK_ALL_STACK_SIZE 1024
#define BLINK_ALL_PRIORITY	30
int blink_init()
{
	rt_mutex_init(&list_lock,"lslock",RT_IPC_FLAG_FIFO);
	rt_thread_t tid;
	rt_err_t result = RT_EOK;
	tid = rt_thread_create("blink_timer",
                           blink_tmr, RT_NULL,
                           BLINK_ALL_STACK_SIZE, BLINK_ALL_PRIORITY, 10);
	
	if (tid != NULL && result == RT_EOK)
	{
        rt_thread_startup(tid);
		return RT_EOK;
	}
    else
	{
		return -RT_ERROR;
	}

}
INIT_ENV_EXPORT(blink_init);

#if defined(RT_USING_FINSH) && defined(FINSH_USING_MSH)
#include <finsh.h>
static void blink_probe(uint8_t argc, char **argv)
{
	rt_slist_t * l = RT_NULL;

	rt_kprintf("%8s %8s %8s %8s \n","  name  ","interval","repCnt", "remCnt");
	rt_kprintf("%8s %8s %8s %8s \n","--------","--------","--------", "--------");
	rt_slist_for_each(l,&blink_devices)						
	{
		struct blink_device_t *bd_temp = rt_slist_entry(l,struct blink_device_t,object_list);
		
		rt_kprintf("%8s %8d %8d %8d \n",bd_temp->name,bd_temp->interval,bd_temp->repeat_cnt, bd_temp->remain_cnt);
	
	}

}
MSH_CMD_EXPORT(blink_probe, list all blink devices);
static void blink_start(uint8_t argc, char **argv)
{
	if(argc == 2)
	{
		blink_device_start(argv[1]);
	}
}
MSH_CMD_EXPORT(blink_start, start a blink device);

static void blink_stop(uint8_t argc, char **argv)
{
	if(argc == 2)
	{
		blink_device_stop(argv[1]);
	}
}
MSH_CMD_EXPORT(blink_stop, stop a blink device);
#endif