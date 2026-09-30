#include "sitronix_ts_custom_func.h"
#include "sitronix_ts.h"
#include "../../../../fih/fih_touch.h"

#ifndef TPD_NO_GPIO
//#include "cust_gpio_usage.h"
#endif
//#include <linux/wakelock.h>

#include <linux/regulator/consumer.h>
#include <linux/of_irq.h>
#include <linux/device.h>

//#define SITRONIX_POWER_SWITCH


#define SWITCH_OFF                  0
#define SWITCH_ON                   1

#define STX_AFE_REG_TYPE	4

#define STX_RESTORE_INT_FUNC

static unsigned int touch_irq = 0;
#define ABS(x) ((x < 0) ? -x : x)
#define TPD_OK 0
#define MAX_BUFFER_SIZE 144
//#define CTP_NAME "CF1xxx"

//#define MAX_BUFFER_SIZE        1028//144
#define MAX_CMD_BUFFER_SIZE 144
#define MAX_FINGER_NUMBER 10
#define MAX_PRESSURE 15
//#define DEVICE_NAME            "CF1xxx"
#define DEVICE_VENDOR 0
#define DEVICE_PRODUCT 0
#define DEVICE_VERSION 0

//add by FHH for I2C DMA mode start
#define MAX_I2C_NDMA_LEN 8
#define MAX_I2C_DMA_LEN 240 //should less than 256
static DEFINE_MUTEX(cf1xxx_i2c_mutex);

//extern struct tpd1_device *tpd1;

static int tpd_flag = 0;

static int g_i2cErrorCount = 0;
#define I2C_CONTINUE_ERROR_CNT	30

#ifdef I2C_SUPPORT_RS_DMA
static u8 *I2CDMABuf_va = NULL;
static u32 I2CDMABuf_pa = NULL;
#endif
static struct task_struct *thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(waiter);

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT] = TPD_WARP_END;
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8] = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif

static irqreturn_t tpd_eint_interrupt_handler(int irq, void *data);
static int touch_event_handler(void *unused);
static int tpd_i2c_probe(struct i2c_client *client);
static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
void tpd_i2c_remove(struct i2c_client *client);
void tpd_down(int raw_x, int raw_y, int x, int y, int p);
void tpd_up(int raw_x, int raw_y, int x, int y, int p);

static struct i2c_client *i2c_client = NULL;
struct sitronix_ts_data stx_gpts = {0};

//#define SITRONIX_SENSOR_KEY

extern struct fih_touch_cb touch_cb;
extern void sitronix_touch_tpfwver_read(char *fw_ver);
extern void touch_selftest(void);
extern int selftest_result_read(void);
extern void stx_vendor_read(char *);

extern int fih_st_tp_chip_type;
int fih_st_tp_chip_type = -1;
//struct pm_qos_request i2c_req;
#ifdef SITRONIX_SENSOR_KEY
struct sitronix_sensor_key_t{
	unsigned int code;
};

#define STX_KEY_NUM_MAX 3
char sitronix_sensor_key_status = 0;
struct sitronix_sensor_key_t sitronix_sensor_key_array[] = {
	{KEY_MENU}, // bit 2 139
	{KEY_HOMEPAGE}, // bit 1 172
	{KEY_BACK}, // bit 0 158
};
#endif

static const struct i2c_device_id tpd_i2c_id[] = {{CTP_NAME, 0}, {}}; // {{"mtk-tpd",0},{}};

static const struct of_device_id tpd_of_match[] = {
    {.compatible = "mediatek,sitronix_ts"},
    {},
};
static struct i2c_driver tpd_i2c_driver = {
    .driver = {
        .name = CTP_NAME,
#ifdef CONFIG_OF
        .of_match_table = tpd_of_match,
#endif
        .owner = THIS_MODULE,
    },
    .probe = tpd_i2c_probe,
    .remove = tpd_i2c_remove,
    .detect = tpd_i2c_detect,
    .driver.name = CTP_NAME, //"mtk-tpd",
    .id_table = tpd_i2c_id,
    //   .address_list 	= forces,
};

static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    //  strcpy(info->type, "mtk-tpd");
    strcpy(info->type, CTP_NAME);
    return 0;
}

void st_reset_ic(void)
{
    tpd1_gpio_output(stx_gpts.host_if->reset_gpio, 1); //ugrec_tky gpio_direction_output(tpd_rst_gpio_number, 0);
    mdelay(10);
    tpd1_gpio_output(stx_gpts.host_if->reset_gpio, 0); //ugrec_tky gpio_direction_output(tpd_rst_gpio_number, 1);
    mdelay(10);
    tpd1_gpio_output(stx_gpts.host_if->reset_gpio, 1); //ugrec_tky gpio_direction_output(tpd_rst_gpio_number, 0);
    mdelay(150);
}

void stx_irq_enable(void)
{
    STX_FUNC_ENTER();
    enable_irq(stx_gpts.irq);
}

void stx_irq_disable(void)
{
    STX_FUNC_ENTER();
    disable_irq_nosync(stx_gpts.irq);
}

static int tpd_irq_registration(void)
{
    struct device_node *node = NULL;
    int ret = 0;
    u32 ints[2] = {0, 0};

    STX_INFO("Device Tree Tpd_irq_registration!");

    node = of_find_matching_node(node, touch_sitronix_of_match);
    if (node)
    {
        of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
        gpiod_set_debounce(gpio_to_desc(ints[0]), ints[1]);

        touch_irq = irq_of_parse_and_map(node, 0);

        stx_gpts.irq = touch_irq;

		tpd1_gpio_as_int(GTP_INT_PORT);
        ret = request_irq(touch_irq, (irq_handler_t)tpd_eint_interrupt_handler, IRQF_TRIGGER_FALLING,
                          "TOUCH_PANEL-eint", NULL); //IRQF_TRIGGER_FALLING  IRQF_TRIGGER_RISING
        if (ret > 0)
        {
            ret = -1;
            STX_ERROR("tpd request_irq IRQ LINE NOT AVAILABLE!.");
        }
    }
    else
    {
        STX_ERROR("tpd request_irq can not find touch eint device node!.");
        ret = -1;
    }

    STX_INFO("[%s]irq:%d, debounce:%d-%d:", __func__, touch_irq, ints[0], ints[1]);
    return ret;
}

static int stx_touch_i2c_check(void)
{
	int ret = 0;
	unsigned char buffer[2];
    STX_INFO("%s", __func__);
	ret = stx_i2c_read_bytes(0x01, buffer, 1);
	if (ret < 0){
		STX_ERROR("i2c read status reg error (%d)", ret);
		return -1;
	}else{
		STX_INFO("check status = %d", buffer[0]);
	}
	return 0;
}

int g_version_select = -1;

static int stx_vendor_ID(void)
{
	int addr = 0;
	unsigned char buf[4] = {0x0};
	int len = 0;
	int type = 0;
	int fa_ver = 0;
	int ret = 0;

	addr = 0x74;
	len = 2;
	ret = stx_cmdio_read(STX_AFE_REG_TYPE, addr,buf,len);
	if(ret <= 0)
	{
		STX_ERROR("cmdio error");
		g_version_select = -1;
		return -1;
	}
	STX_INFO("addr[0x%x]:0x%x,0x%x ",addr,buf[0],buf[1]);

	type = buf[1] & 0x3f;
	fa_ver = (buf[1] >> 6) & 0x03;
	if((type > 0x01)&&(type < 0x0E)&&(fa_ver == 0x0))
	{
		STX_INFO("ST14348");
		g_version_select = ST14348_VERSION;
		fih_st_tp_chip_type = 0;
	}else{
		STX_INFO("ST14348B");
		g_version_select = ST14348_B_VERSION;
		fih_st_tp_chip_type = 1;
	}
	return 0;
}

void sitronix_touch_tpfwver_read(char *fw_ver)
{
    pr_err("%s enter\n",__func__);
    memset(fw_ver, 0, sizeof(*fw_ver));
    snprintf(fw_ver, 128, "Sitronix-V%X.%s\n", stx_gpts.ts_dev_info.fw_version[0], stx_gpts.ts_dev_info.fw_revision);
}

void stx_vendor_read(char *id)
{
    pr_err("%s enter\n",__func__);
    if(fih_st_tp_chip_type == 0)
        snprintf(id, 128, "ST14348\n");
    else if(fih_st_tp_chip_type == 1)
        snprintf(id, 128, "ST14348B\n");
    else
        snprintf(id, 128, "Unknown\n");
}

#include "../touch_protect.h"

static int tpd_i2c_probe(struct i2c_client *client)
{
    int err = 0; //, ret = -1
                 //    int status = 0;
#if 0
    int retval;
#endif
#ifdef SITRONIX_SENSOR_KEY
    int i;
#endif
	struct sitronix_ts_platform_data *pdata;

	pdata = devm_kzalloc(&client->dev,
						 sizeof(struct sitronix_ts_platform_data),
						 GFP_KERNEL);
	if (!pdata) 
	{
		STX_ERROR("TP Failed to allocate memory for pdata");
		return -ENOMEM;
	}
	stx_gpts.host_if = pdata;
    i2c_client = client;
    stx_gpts.client = client;
    stx_gpts.suspend_state = 0;
    stx_gpts.input_dev = tpd1->dev; //hfst add
    stx_gpts.host_if->reset_gpio = GTP_RST_PORT;
    stx_gpts.host_if->irq_gpio = 60;
    stx_gpts.fsmart_wakeup = 1;
    stx_gpts.is_upgrading = false;
    stx_gpts.is_testing = false;
    stx_gpts.fapp_in = 0;
    stx_gpts.glove_mode = false;
    stx_gpts.cases_mode = false;
	stx_gpts.host_if->default_i2c_flag = 0;

    mutex_init(&stx_gpts.dev_mutex);
	mutex_lock(&touch_lock);
    //STX_INFO("Sitronix touch panel i2c probe:%s", id->name);
	if(touch_init) {
		STX_INFO("%s: aleady to initial the other touch driver\n", __func__);
		return -1;
	}
#if 0
    retval = regulator_enable(tpd1->reg);
    if (retval != 0)
        TPD_DMESG("Failed to enable reg-vgp6: %d", retval);
#endif
    tpd_sitronix_gpio_power_enable(1);
    msleep(5);
    st_reset_ic();

#ifdef I2C_SUPPORT_RS_DMA
    I2CDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &I2CDMABuf_pa, GFP_KERNEL);
    if (!I2CDMABuf_va)
    {
        STX_ERROR("sitronix Allocate Touch DMA I2C Buffer failed!");
		mutex_unlock(&touch_lock);
        return -1;
    }
#endif
    i2c_client->addr = 0x55;
	err = stx_touch_i2c_check();
#ifdef ST_UPGRADE_FIRMWARE
	if(err < 0)
	{
		STX_ERROR("status error ,try upgrade");
		st_upgrade_fw();
		err = stx_touch_i2c_check();
	}
#endif
	if(err < 0)
	{
		STX_ERROR("sitronix check error in probe ,return");
        //tpd1_gpio_output(stx_gpts.host_if->reset_gpio, 0);
        //msleep(1);

#if 0
        regulator_disable(tpd1->reg);
		regulator_put(tpd1->reg);
#endif
        //tpd_sitronix_gpio_power_enable(0);
		mutex_unlock(&touch_lock);
		return -1;
	}

    st_create_sysfs(client);

#ifdef ST_DEVICE_NODE
    st_dev_node_init();
#endif

#ifdef SITRONIX_SENSOR_KEY
	for (i = 0; i < STX_KEY_NUM_MAX; i++) {
		input_set_capability(tpd1->dev, EV_KEY, sitronix_sensor_key_array[i].code);
	}
#endif
    stx_vendor_ID();
    if(fih_st_tp_chip_type == 0){
        STX_ERROR(" sitronix ST14348 probe fail\n");
        tpd1_gpio_output(stx_gpts.host_if->reset_gpio, 0);
        msleep(1);
        tpd_sitronix_gpio_power_enable(0);
		mutex_unlock(&touch_lock);
        return -1;
    }

    tpd_irq_registration();
    //enable_irq(touch_irq);
    //STX_DEBUG("MediaTek sitronix touch panel i2c probe success");
    msleep(100);

    st_gesture_init();

#ifdef ST_UPGRADE_FIRMWARE
    kthread_run(st_upgrade_fw_handler, "Sitronix", "sitronix_update");
#else
    st_print_version(&stx_gpts);
#endif //ST_UPGRADE_FIRMWARE

    thread = kthread_run(touch_event_handler, 0, CTP_NAME);
    if (IS_ERR(thread))
    {
        err = PTR_ERR(thread);
        STX_ERROR(" sitronix failed to create kernel thread: %d", err);
    }

    tpd1_load_status = 1;
	//pm_qos_add_request(&i2c_req, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);
#ifdef ST_MONITOR_THREAD
    sitronix_monitor_start();
#endif

#ifndef ST_UPGRADE_FIRMWARE
	st_get_touch_info(&stx_gpts);
#endif
    touch_cb.touch_tpfwver_read = sitronix_touch_tpfwver_read;
    touch_cb.touch_selftest = touch_selftest;
    touch_cb.touch_selftest_result = selftest_result_read;
    touch_cb.touch_vendor_read = stx_vendor_read;
	mutex_unlock(&touch_lock);

    return 0;
}

#ifdef SITRONIX_SENSOR_KEY
static inline void sitronix_ts_handle_sensor_key(struct sitronix_sensor_key_t *key_array,
                                                char *pre_key_status, char cur_key_status, int key_count)
{
	int i = 0;
	for(i = 0; i < key_count; i++){
		if(cur_key_status & (1 << i)){
			STX_INFO("sensor key cur_key_status:%d,i=%d \n",cur_key_status,i);
			input_report_key(tpd1->dev, key_array[i].code, 1);
			input_sync(tpd1->dev);
		}else{
			if(*pre_key_status & (1 << i)){
				STX_INFO("sensor key [%d] up\n", i);
				input_report_key(tpd1->dev, key_array[i].code, 0);
				input_sync(tpd1->dev);
			}
		}
	}
	*pre_key_status = cur_key_status;
}
#endif

#ifdef STX_RESTORE_INT_FUNC

#define	INT_OCCURRED_TRUE	1
#define	INT_OCCURRED_FALSE	0

#define	INTERVAL_MS_OF_INT_OCCURRED	80
#define INT_RESTORE_RETRY_TIMES		10

static atomic_t gIntOccurred = ATOMIC_INIT(0);
static struct task_struct *StxIntRestoreThread = NULL;

static void stx_int_occurred_set(int flag)
{
	atomic_set(&gIntOccurred, flag);
}

static int stx_int_occurred_get(void)
{
	int flag = 0;
	flag = atomic_read(&gIntOccurred);
	return flag;
}

static int stx_int_restore_thread(void *data)
{
	int gpio_status = -1;
	int i;
	int ret;
	u16 x, y;
	uint8_t buf[42] = {0};
	uint8_t PixelCount = 0;
	int retry = INT_RESTORE_RETRY_TIMES;

	while (!kthread_should_stop())
	{
		if(stx_int_occurred_get() == INT_OCCURRED_TRUE)
			stx_int_occurred_set(INT_OCCURRED_FALSE);

		msleep(INTERVAL_MS_OF_INT_OCCURRED);

		if(stx_int_occurred_get() == INT_OCCURRED_FALSE)
		{
			STX_INFO("sitronix_INT it is the last INT");
			gpio_status = gpio_get_value(stx_gpts.host_if->irq_gpio);

			if(gpio_status == 1)
			{
				usleep_range(100, 100);
				gpio_status = gpio_get_value(stx_gpts.host_if->irq_gpio);
			}

//			if(gpio_status == 0)
			while((gpio_status == 0) &&(retry > 0) &&(stx_int_occurred_get() == INT_OCCURRED_FALSE))
			{
				STX_INFO("sitronix_INT the INT GPIO is low");
				sitronix_monitor_delay();

				ret = stx_i2c_read(stx_gpts.client, buf, stx_gpts.ts_dev_info.max_touches * 4, 0x11);
				if (ret < 0)
				{
					STX_ERROR("read finger error (%d)", ret);
					return ret;
				}

				for (i = 0; i < stx_gpts.ts_dev_info.max_touches; i++)
				{
					if (buf[1 + 4 * i] & 0x80)
					{
						x = (int)(buf[1 + i * 4] & 0x70) << 4 | buf[1 + i * 4 + 1];
						y = (int)(buf[1 + i * 4] & 0x0F) << 8 | buf[1 + i * 4 + 2];

						PixelCount++;
						STX_DEBUG("SITRONIX Touch Point: %d (%d,%d)", i, x, y);
						tpd_down(0, 0, x, y, i);
					}
					else
					{
						tpd_up(0, 0, 0, 0, i);
					}
				}
				input_report_key(stx_gpts.input_dev, BTN_TOUCH, PixelCount > 0);
				input_sync(stx_gpts.input_dev);

				usleep_range(5, 5);
				gpio_status = gpio_get_value(stx_gpts.host_if->irq_gpio);
				retry--;
			}
			StxIntRestoreThread = NULL;
			return 0;
		}
	}
	STX_FUNC_EXIT();
	return 0;
}

static int stx_int_restore_start(void)
{
	int err = 0;

	if(NULL == StxIntRestoreThread)
	{
		STX_INFO("%s ENTER ",__func__);

		StxIntRestoreThread = kthread_run(stx_int_restore_thread, (void *)NULL, "SitronixIntThread");
		if (IS_ERR(StxIntRestoreThread))
		{
			err = PTR_ERR(StxIntRestoreThread);
			StxIntRestoreThread = NULL;
			STX_ERROR("%s ERROR %d",__func__,err);
			return err;
		}
		STX_INFO("%s success ",__func__);
	}else{
		//STX_INFO("%s already start ",__func__);
	}
	return 0;
}
#endif

static irqreturn_t tpd_eint_interrupt_handler(int irq, void *data)
{
    //TPD_DEBUG_PRINT_INT;
    tpd_flag = 1;
    disable_irq_nosync(stx_gpts.irq);
#ifdef STX_RESTORE_INT_FUNC
	stx_int_occurred_set(INT_OCCURRED_TRUE);
#endif
    wake_up_interruptible(&waiter);
    return IRQ_HANDLED;
}

void tpd_i2c_remove(struct i2c_client *client)
{
    st_remove_sysfs(client);

#ifdef I2C_SUPPORT_RS_DMA
    if (I2CDMABuf_va)
    {
        dma_free_coherent(NULL, 4096, I2CDMABuf_va, I2CDMABuf_pa);
        I2CDMABuf_va = NULL;
        I2CDMABuf_pa = 0;
    }
#endif
	//pm_qos_remove_request(&i2c_req);
#ifdef ST_MONITOR_THREAD
    sitronix_monitor_stop();
#endif

    return;
}

void tpd_down(int raw_x, int raw_y, int x, int y, int p)
{
#ifdef CONFIG_GTP_ICS_SLOT_REPORT
    input_mt_slot(tpd1->dev, p);
    input_mt_report_slot_state(tpd1->dev, MT_TOOL_FINGER, 1);
    input_report_abs(tpd1->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd1->dev, ABS_MT_POSITION_Y, y);
    input_report_abs(tpd1->dev, ABS_MT_TOUCH_MAJOR, 255);
//	input_report_key(tpd1->dev, BTN_TOUCH, 1);
#else
    input_report_abs(tpd1->dev, ABS_MT_TRACKING_ID, p + 1);
//    input_report_abs(tpd1->dev, ABS_MT_PRESSURE, 128);
//    input_report_key(tpd1->dev, BTN_TOUCH, 1);
//    input_report_abs(tpd1->dev, ABS_MT_TOUCH_MAJOR, 128);
//    input_report_abs(tpd1->dev, ABS_MT_WIDTH_MAJOR, 128);
    input_report_abs(tpd1->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd1->dev, ABS_MT_POSITION_Y, y);

    input_mt_sync(tpd1->dev);
#endif
	STX_DEBUG("sitronix down: [%d](%d, %d)+", p, x, y);
}

void tpd_up(int raw_x, int raw_y, int x, int y, int p)
{
#ifdef CONFIG_GTP_ICS_SLOT_REPORT
    input_mt_slot(tpd1->dev, p);
    input_mt_report_slot_state(tpd1->dev, MT_TOOL_FINGER, 0);
//	input_report_key(tpd1->dev, BTN_TOUCH, 0);
#else
    input_report_abs(tpd1->dev,  ABS_MT_TRACKING_ID, -1);
//    input_report_abs(tpd1->dev, ABS_MT_PRESSURE, 0);
//    input_report_key(tpd1->dev, BTN_TOUCH, 0);
//    input_report_abs(tpd1->dev, ABS_MT_TOUCH_MAJOR, 0);
//    input_report_abs(tpd1->dev, ABS_MT_WIDTH_MAJOR, 0);
    input_report_abs(tpd1->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd1->dev, ABS_MT_POSITION_Y, y);
    input_mt_sync(tpd1->dev);
#endif

	STX_DEBUG("up :[%d]-", p);
}

static void sitronix_ts_pen_allup(void)
{
#ifdef CONFIG_GTP_ICS_SLOT_REPORT
	int i;
    for(i=0; i <= ST_MAX_TOUCHES; i++)
    {
        input_mt_slot(tpd1->dev, i);
//        input_report_abs(tpd1->dev, ABS_MT_TRACKING_ID, -1);
		input_report_key(tpd1->dev, BTN_TOUCH, 0);
        input_mt_report_slot_state(tpd1->dev, MT_TOOL_FINGER, 0);
    }
#else
    input_report_key(tpd1->dev, BTN_TOUCH, 0);
    input_mt_sync(tpd1->dev);
#endif
    input_sync(tpd1->dev);
}

static int touch_event_handler(void *unused)
{
    //struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
    unsigned char buf[ST_MAX_TOUCHES * 4];
    int ret = 0, i = 0;
    int x, y;
    u8 touchCount = 0;
    static u16 pre_index = 0; 
    struct sched_param param = {.sched_priority = 4};

    sched_setscheduler(current, SCHED_RR, &param);
    do
    {
        set_current_state(TASK_INTERRUPTIBLE);

        wait_event_interruptible(waiter, tpd_flag != 0);

#ifdef ST_MONITOR_THREAD
        sitronix_monitor_delay();
#endif
        tpd_flag = 0;
        //TPD_DEBUG_SET_TIME;
        set_current_state(TASK_RUNNING);

#ifdef ST_SMART_WAKE_UP
        if (stx_gpts.fsmart_wakeup == 1)
        {
            ret = sitronix_swk_func(tpd1->dev);
            if (ret == 0)
            {
                enable_irq(stx_gpts.irq);
                continue;
            }
        }
#endif //ST_SMART_WAKE_UP
        touchCount = 0;
        ret = stx_i2c_read(i2c_client, buf, (ST_MAX_TOUCHES * 4+1), 0x11);
		if (ret < 0)
		{
			STX_ERROR("read finger error (%d)", ret);
			g_i2cErrorCount++;
		}else{
			g_i2cErrorCount = 0;
		}

        for (i = 0; i < ST_MAX_TOUCHES; i++)
        {
            if (buf[1 + 4 * i] & 0x80)
            {
                x = (int)(buf[1 + i * 4] & 0x70) << 4 | buf[1 + i * 4 + 1];
                y = (int)(buf[1 + i * 4] & 0x0F) << 8 | buf[1 + i * 4 + 2];
                if (sitronix_cases_mode_check(x, y))
                {
                    touchCount++;
                    STX_DEBUG("Sitronix touch point: %d (%d,%d)", i, x, y);
                    tpd_down(0, 0, x, y, i);
                    pre_index |= 0x01 << i;
                }
            }else if(pre_index & (0x01 << i))
            {
                pre_index &= ~(0x01 << i);
#ifdef CONFIG_GTP_ICS_SLOT_REPORT
                tpd_up(0, 0, 0, 0, i);
#endif
            }
        }
#ifndef CONFIG_GTP_ICS_SLOT_REPORT
        if (touchCount == 0)
        {
            tpd_up(0, 0, 0, 0, 0);
        }
#endif
		input_report_key(tpd1->dev, BTN_TOUCH, touchCount> 0);
        input_sync(tpd1->dev);
#ifdef SITRONIX_SENSOR_KEY
		sitronix_ts_handle_sensor_key(sitronix_sensor_key_array, &sitronix_sensor_key_status, buf[0], (sizeof(sitronix_sensor_key_array)/sizeof(struct sitronix_sensor_key_t)));
#endif

		if(g_i2cErrorCount >= I2C_CONTINUE_ERROR_CNT){
			STX_ERROR("I2C abnormal in work_func(), reset it! ");
			st_reset_ic();
			g_i2cErrorCount = 0;
		}

        enable_irq(stx_gpts.irq);
#ifdef STX_RESTORE_INT_FUNC
		stx_int_restore_start();
		stx_int_occurred_set(INT_OCCURRED_TRUE);
#endif
    } while (!kthread_should_stop());

    return 0;
}

int tpd_local_init(void)
{
#if 0
    int ret;
#endif

#if !defined CONFIG_MTK_LEGACY
#if 0
    tpd1->reg = regulator_get(tpd1->tpd_dev, "vtouch"); // get pointer to regulator structure
    if (IS_ERR(tpd1->reg))
    {
        STX_ERROR("regulator_get() failed!");
    }

    ret = regulator_set_voltage(tpd1->reg, 2800000, 2800000); // set 2.8v
    if (ret)
        STX_ERROR("regulator_set_voltage() failed!");
    ret = regulator_enable(tpd1->reg); //enable regulator
    if (ret)
        STX_ERROR("regulator_enable() failed!");
#endif
    tpd_sitronix_gpio_power_enable(1);

#endif

    //spin_lock_init(&irq_flag_lock);
    if (i2c_add_driver(&tpd_i2c_driver) != 0)
    {
        STX_ERROR("unable to add i2c driver.");
        return -1;
    }

    if (tpd1_load_status == 0)
    {
        STX_ERROR("add error touch panel driver.");
        i2c_del_driver(&tpd_i2c_driver);

        return -1;
    }

#ifdef CONFIG_GTP_ICS_SLOT_REPORT
	input_mt_init_slots(tpd1->dev, 10, 0);
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
    memcpy(tpd_calmat, tpd_def_calmat_local, 8 * 4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local, 8 * 4);
#endif
    STX_INFO("end %s, %d", __FUNCTION__, __LINE__);

    //tpd_type_cap = 1;

    return 0;
}

#ifdef SITRONIX_POWER_SWITCH
int stx_power_switch(int state)
{
    static int power_state = 0;
	int ret = 0;

	switch (state) {
	case SWITCH_ON:
        if (power_state == 0) {
    		STX_INFO("Power switch on!");
            tpd_sitronix_gpio_power_enable(1);
#if 0
			ret = regulator_enable(tpd1->reg);	/*enable regulator*/	
			if (ret)
				STX_ERROR("regulator_enable() failed!\n");
#endif
            power_state = 1;
        }
    		break;
	case SWITCH_OFF:
        if (power_state == 1) {
		    STX_INFO("Power switch off!");
            tpd_sitronix_gpio_power_enable(0);
#if 0
            ret = regulator_disable(tpd1->reg);	/*disable regulator*/
            if (ret)
                STX_ERROR("regulator_disable() failed!\n");
#endif
            power_state = 0;
        }
		break;
	default:
		STX_ERROR("Invalid power switch command!");
		break;
	}
	return 0;
}
#endif

/*static void stx_shutdown(void)
{
    STX_FUNC_ENTER();
    tpd1_gpio_output(stx_gpts.host_if->reset_gpio, 0);
    msleep(5);
    tpd_sitronix_gpio_power_enable(0);
    STX_FUNC_EXIT();
}*/

/* Function to manage low power suspend */

void sitronix_suspend(void)
{
    STX_FUNC_ENTER();

    if(stx_gpts.fapp_in)
    {
        STX_INFO("%s fapp_in = %d",__func__,stx_gpts.fapp_in);
        return;
    }
#ifdef ST_MONITOR_THREAD
    sitronix_monitor_stop();
#endif

    if (stx_gpts.glove_mode)
    {
        st_enter_glove_mode(&stx_gpts);
    }

#ifdef ST_SMART_WAKE_UP
    if (stx_gpts.fsmart_wakeup == 1)
    {
        sitronix_swk_enable();
        st_power_down(&stx_gpts);
        enable_irq_wake(stx_gpts.irq);
        sitronix_ts_pen_allup();
        stx_gpts.suspend_state = 1;
        return;
    }
#endif
    disable_irq(touch_irq);
    st_power_down(&stx_gpts);
    sitronix_ts_pen_allup();

#ifdef SITRONIX_POWER_SWITCH
    tpd1_gpio_output(stx_gpts.host_if->reset_gpio, 0);
    msleep(1);
	stx_power_switch(SWITCH_OFF);
#endif
    stx_gpts.suspend_state = 1;

    STX_FUNC_EXIT();
    return;
}

static void tpd_suspend(struct device *h)
{
	sitronix_suspend();
}

void sitronix_resume(void)
{
    STX_FUNC_ENTER();
    if(stx_gpts.fapp_in)
    {
        STX_INFO("%s fapp_in = %d",__func__,stx_gpts.fapp_in);
        return;
    }

    tpd1_gpio_output(stx_gpts.host_if->reset_gpio, 0);
    msleep(5);
#ifdef SITRONIX_POWER_SWITCH
	stx_power_switch(SWITCH_ON);
#endif
    msleep(10);
    tpd1_gpio_output(stx_gpts.host_if->reset_gpio, 1);

//    st_power_up(&stx_gpts);

    if (stx_gpts.glove_mode)
    {
		msleep(150);
        st_enter_glove_mode(&stx_gpts);
    }

#ifdef ST_MONITOR_THREAD
    sitronix_monitor_start();
#endif

#ifdef ST_SMART_WAKE_UP
    if (stx_gpts.fsmart_wakeup == 1)
    {
        sitronix_swk_disable();
        disable_irq_wake(stx_gpts.irq);
        stx_gpts.suspend_state = 0;
        return;
    }
#endif
    stx_gpts.suspend_state = 0;
    enable_irq(touch_irq);
    STX_FUNC_EXIT();
    return;
}

/* Function to manage power-on resume */
static void tpd_resume(struct device *h)
{
	sitronix_resume();
}

static struct tpd1_driver_t tpd_device_driver = {
    .tpd1_device_name = CTP_NAME, // TPD_DEVICE,
    .tpd1_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
    //.shutdown = stx_shutdown,
#ifdef TPD_HAVE_BUTTON
    .tpd1_have_button = 1,
#else
    .tpd1_have_button = 0,
#endif
};

/* called when loaded into kernel */
void sitronix_touch_driver_init(void)
{
    tpd1_get_dts_info();
    STX_INFO("Sitronix touch panel driver init");

    if (tpd1_driver_add(&tpd_device_driver) < 0)
        STX_ERROR("add generic driver failed");
}

/* should never be called */
void sitronix_touch_driver_exit(void)
{
    STX_INFO("MediaTek SITRONIX touch panel driver exit");
    //input_unregister_device(tpd1->dev);
    tpd1_driver_remove(&tpd_device_driver);
#ifdef ST_DEVICE_NODE
    st_dev_node_exit();
#endif
}

EXPORT_SYMBOL_GPL(sitronix_touch_driver_init);
EXPORT_SYMBOL_GPL(sitronix_touch_driver_exit);

MODULE_AUTHOR("SITRONIX Driver Team");
MODULE_DESCRIPTION("SITRONIX Touchscreen Driver");
MODULE_LICENSE("GPL v2");
