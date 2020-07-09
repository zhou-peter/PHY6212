#include <aos/ble.h>
#include "gap.h"
#include <log.h>

#define TAG "GAP"

ble_gap_state_t g_gap_data;

aos_timer_t adv_timer = {0};

// https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.gap.appearance.xml
// static uint8_t appearance_hid[] = {0xc0, 0x03};         // @key:960 @value:Human Interface Device (HID) @description:HID Generic
// static uint8_t appearance_Keyboard[] = {0xc1, 0x03};    // @key:961 @value:Keyboard @description:HID subtype
// static uint8_t appearance_mouse[] = {0xc2, 0x03};       // @key:962 @value:Mouse @description:HID subtype
// static uint8_t adv_uuids[] = {0x0f, 0x18};

static ad_data_t app_scan_rsp_data[1] = {
    [0] = {
        .type = AD_DATA_TYPE_GAP_APPEARANCE,
        .data = (uint8_t[]){0xc1, 0x03},            // keyboard:961
        .len = 2
    }
};

static ad_data_t app_adv_data[4] = {
    [0] = {
        .type = AD_DATA_TYPE_FLAGS,
        .data = (uint8_t[]){AD_FLAG_GENERAL | AD_FLAG_NO_BREDR},
        .len = 1
    },
    [1] = {
        .type = AD_DATA_TYPE_UUID16_ALL,
        .data = (uint8_t[]){0x0f, 0x18},            // uuid_hids
        .len = 2,
    },
    [2] = {
        .type = AD_DATA_TYPE_NAME_COMPLETE,
        .data = (uint8_t *)DEVICE_NAME,
        .len = sizeof(DEVICE_NAME),
    },
    [3] = {
        .type = AD_DATA_TYPE_GAP_APPEARANCE,
        .data = (uint8_t[]){0xc1, 0x03},            // keyboard:961
        .len = 2
    }
};



bool stop_adv()
{
    if (ble_stack_adv_stop() != 0) {
        LOGE("GAP", "failed to stop adversting");
        return false;
    } else {
        return true;
    }
    // if (adv_timer.hdl.timer_state == TIMER_ACTIVE) {
    //     aos_timer_stop(&adv_timer);
    // }
}

static void adv_timer_callback(void *arg1, void *arg2)
{
    LOGI("GAP", "adv_timer_callback");

    if (g_gap_data.state == GAP_STATE_ADVERTISING) {
        if (stop_adv() == true) {
            LOGI("GAP", "advertising stoped");
            g_gap_data.state = GAP_STATE_IDLE;
        } else {
            LOGE("GAP", "advertising cant not been stoped");
        }
    }
}

static bool start_adv(adv_type_t type)
{
    int adv_timeout = 0;
    adv_param_t param = {
        .type = ADV_IND,
        .ad = app_adv_data,
        .sd = app_scan_rsp_data,
        .ad_num = BLE_ARRAY_NUM(app_adv_data),
        .sd_num = BLE_ARRAY_NUM(app_scan_rsp_data),
        .filter_policy = ADV_FILTER_POLICY_ANY_REQ,
        .channel_map = ADV_DEFAULT_CHAN_MAP,
        .direct_peer_addr = {0},
        // .interval_min = ADV_FAST_INT_MIN_1,
        // .interval_max = ADV_FAST_INT_MAX_1,
        .interval_min = ADV_FAST_INT_MIN_2,
        .interval_max = ADV_FAST_INT_MAX_2,
    };

    // 根据不同的广播类型配置广播参数
    g_gap_data.ad_type = ADV_TYPE_IDLE;
    switch (type)
    {
        // 回连广播
        case ADV_TYPE_RECONNECT_NORMAL:
            // param.type = ADV_DIRECT_IND;
            param.type = ADV_IND;
            param.direct_peer_addr = bond_info.remote_addr;
            // param.filter_policy = ADV_FILTER_POLICY_CONN_REQ;    // 无法工作
            param.filter_policy =  ADV_FILTER_POLICY_ANY_REQ;
            param.interval_min = ADV_FAST_INT_MIN_2;
            param.interval_max = ADV_FAST_INT_MAX_2;
            adv_timeout = ADV_RECONNECT_NORMAL_TIMEOUT;
            break;
        // 配对广播
        case ADV_TYPE_PAIRING:
            param.type = ADV_IND;
            param.filter_policy = ADV_FILTER_POLICY_ANY_REQ;
            param.interval_min = ADV_FAST_INT_MIN_2;
            param.interval_max = ADV_FAST_INT_MAX_2;
            adv_timeout = ADV_PAIRING_TIMEOUT;
            break;
        case ADV_TYPE_INDIRECT_RECONNECT:

            break;
        default:
            break;
    }
    // 启动广播
    if (ble_stack_adv_start(&param) != 0) {   
        // 广播失败
        LOGE(TAG, "advertising start failed!");
        return false;
    } else {
        // 广播成功，启动停止定时器
        g_gap_data.state = GAP_STATE_ADVERTISING;
        g_gap_data.ad_type = type;
        int s;
        if (adv_timer.hdl.cb == NULL) {
            s = aos_timer_new_ext(&adv_timer, adv_timer_callback, NULL, adv_timeout, 0, 1);
        } else {
            s = aos_timer_stop(&adv_timer);
            s = aos_timer_change_without_repeat(&adv_timer, adv_timeout);
            s = aos_timer_start(&adv_timer);
        }
        if (s == 0) {
            LOGE(TAG, "advertising started!, will stop after %d ms", adv_timeout);
        } else {
            LOGE(TAG, "advertising started!, but stop timer can`t work:%d", s);
        }
        return true;
        // led_set_status(BLINK_FAST);
    }
}


#define KEY_BOND_INFO   "BOND"
bond_info_t bond_info = {0};

static bool _save_bond_info()
{
    if (aos_kv_set(KEY_BOND_INFO, (void *)&bond_info, sizeof(bond_info_t), 1) != 0) {
        // TO-DO: 保存失败
        return false;
    }
    return true;
}

static bool _load_bond_info()
{
    int len;
    if (aos_kv_get(KEY_BOND_INFO, (void *)&bond_info, &len) != 0) {
        return false;
    }

    if (len == sizeof(bond_info_t)) {
        return true;
    } else {
        return false;
    }
}

static bool _remove_bond_info()
{
    if (bond_info.is_bonded) {
        ble_stack_dev_unpair(&bond_info.remote_addr);
        ble_stack_white_list_remove(&bond_info.remote_addr);
    }
    bond_info.is_bonded = 0;
    if (aos_kv_set(KEY_BOND_INFO, (void *)&bond_info, sizeof(bond_info_t), 1) != 0) {
        // TO-DO: 保存失败
        return false;
    }
    return true;
}


void rcu_ble_pairing()
{
    if(g_gap_data.state == GAP_STATE_ADVERTISING) {
        aos_timer_stop(&adv_timer);
        aos_timer_change_without_repeat(&adv_timer, ADV_PAIRING_TIMEOUT);
    }

    if(g_gap_data.state == GAP_STATE_IDLE) {
        if(bond_info.is_bonded) {
            start_adv(ADV_TYPE_RECONNECT_NORMAL);
        } else {
            start_adv(ADV_TYPE_PAIRING);
        }
    }

    if(g_gap_data.state == GAP_STATE_CONNECTED || g_gap_data.state == GAP_STATE_PAIRED) {
        _remove_bond_info();
        ble_stack_disconnect(g_gap_data.conn_handle);
    }
}




static void gap_event_smp_pairing_passkey_display(evt_data_smp_passkey_display_t *event_data)
{
    LOGI(TAG, "passkey is %s", event_data->passkey);
}

static void gap_event_smp_pairing_complete(evt_data_smp_pairing_complete_t *event_data)
{
    if (event_data->err == 0)
    {
        g_gap_data.state = GAP_STATE_PAIRED;
        g_gap_data.paired_addr = event_data->peer_addr;
        if (event_data->bonded ) {

            ble_stack_white_list_clear();
            ble_stack_white_list_add(&event_data->peer_addr);

            bond_info.is_bonded = true;
            bond_info.remote_addr = event_data->peer_addr;
            _save_bond_info();
            LOGI(TAG, "bond with remote device:%02x-%02x-%02x-%02x-%02x-%02x",   
                                                    event_data->peer_addr.val[0],
                                                    event_data->peer_addr.val[1],
                                                    event_data->peer_addr.val[2],
                                                    event_data->peer_addr.val[3],
                                                    event_data->peer_addr.val[4],
                                                    event_data->peer_addr.val[5]
                                                    );
        }
    }

    LOGI(TAG, "pairing %s!!!", event_data->err ? "FAIL" : "SUCCESS");
}

static void gap_event_smp_cancel(void *event_data)
{
    LOGI(TAG, "pairing cancel");
}

static void gap_event_smp_pairing_confirm(evt_data_smp_pairing_confirm_t *e)
{
    ble_stack_smp_passkey_confirm(e->conn_handle);    
    LOGI(TAG, "Confirm pairing for");
}

static void gap_event_conn_security_change(evt_data_gap_security_change_t *event_data)
{
    LOGI(TAG, "conn %d security level change to level %d", event_data->conn_handle, event_data->level);
}

static void gap_event_conn_change(evt_data_gap_conn_change_t *event_data)
{
    if (event_data->connected == CONNECTED && event_data->err == 0) {
        // 设置连接加密
        ble_stack_security(event_data->conn_handle, SECURITY_LOW);
        // ble_stack_security(e->conn_handle, SECURITY_MEDIUM);
        g_gap_data.conn_handle = event_data->conn_handle;
        g_gap_data.state = GAP_STATE_CONNECTED;

        aos_timer_stop(&adv_timer);
        // led_set_status(BLINK_SLOW);
        LOGI(TAG, "Connected");
    } else {
        g_gap_data.conn_handle = -1;
        g_gap_data.state = GAP_STATE_DISCONNECTING;
        if (bond_info.is_bonded) {
            start_adv(ADV_TYPE_RECONNECT_NORMAL);
        } else {
            start_adv(ADV_TYPE_PAIRING);
        }

        LOGI(TAG, "Disconnected err %d", event_data->err);

    }
}

static void gap_event_conn_param_update(evt_data_gap_conn_param_update_t *event_data)
{
    LOGD(TAG, "LE conn param updated: int 0x%04x lat %d to %d\n", 
            event_data->interval,
            event_data->latency,
            event_data->timeout);
}

static void gap_event_mtu_exchange(evt_data_gatt_mtu_exchange_t *event_data)
{
    if (event_data->err == 0) {
        g_gap_data.mtu_size = ble_stack_gatt_mtu_get(event_data->conn_handle);
        LOGI(TAG, "mtu exchange, MTU %d", g_gap_data.mtu_size);
    } else {
        LOGE(TAG, "mtu exchange fail, %x", event_data->err);
    }
}

static int gap_event_callback(ble_event_en event, void *event_data)
{
    LOGD(TAG, "GAP event %x\n", event);

    switch (event) {
        case EVENT_GAP_CONN_CHANGE:
            gap_event_conn_change(event_data);
            break;

        case EVENT_GAP_CONN_PARAM_UPDATE:
            gap_event_conn_param_update(event_data);
            break;

        case EVENT_SMP_PASSKEY_DISPLAY:
            gap_event_smp_pairing_passkey_display(event_data);
            break;

        case EVENT_SMP_PAIRING_COMPLETE:
            gap_event_smp_pairing_complete(event_data);
            break;

        case EVENT_SMP_PAIRING_CONFIRM:
            gap_event_smp_pairing_confirm(event_data);
            break;

        case EVENT_SMP_CANCEL:
            gap_event_smp_cancel(event_data);
            break;

        case EVENT_GAP_CONN_SECURITY_CHANGE:
            gap_event_conn_security_change(event_data);
            break;

        case EVENT_GATT_MTU_EXCHANGE:
            gap_event_mtu_exchange(event_data);
            break;

        case EVENT_GAP_ADV_TIMEOUT:
            // start_adv();
            LOGI(TAG, "advertising timerout");  // 什么情况下会发生?
            break;
        default:
            break;
    }

    return 0;
}

static ble_event_cb_t ble_cb = {
    .callback = gap_event_callback,
};

bool rcu_ble_init()
{
    int ret = 0;
    // dev_addr_t addr = {DEV_ADDR_LE_PUBLIC, DEVICE_ADDR};
    dev_addr_t addr = {DEV_ADDR_LE_RANDOM, DEVICE_ADDR};

    init_param_t init = {
        .dev_name = DEVICE_NAME,
        .dev_addr = &addr,
        .conn_num_max = 1,
    };

    ret = ble_stack_init(&init);

    ret = ble_stack_setting_load();

    ret = ble_stack_event_register(&ble_cb);

    ret = ble_stack_iocapability_set(IO_CAP_IN_NONE | IO_CAP_OUT_NONE);

    if (ret != 0) {
        LOGE(TAG, "ble stack init failed");
    }

    hid_service_init();
    dis_service_init();
    battary_service_init();
    atvv_service_init();

    _load_bond_info();
    if (bond_info.is_bonded == true) {
        start_adv(ADV_TYPE_RECONNECT_NORMAL);
    } else {
        start_adv(ADV_TYPE_PAIRING);
    }

    return true;
}