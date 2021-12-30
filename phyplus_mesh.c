#include <stdio.h>
#include <string.h>
#include "mible_mesh_api.h"
#include "mi_config.h"
#include "mible_log.h"
#include "mible_api.h"
#include "MS_access_api.h"
#include "MS_net_api.h"

#include "access_extern.h"
#include "net_extern.h"
#include "MS_config_api.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "hci.h"
#include "hci_tl.h"
#include "ll.h"
#include "gap.h"

#include "gapgattserver.h"
#include "gattservapp.h"
#include "devinfoservice.h"
#include "bleMesh.h"
#include "mible_log.h"

//#include <trace.h>
//#include "mesh_api.h"
//#include "mesh_sdk.h"
//#include "mesh_node.h"
//#include "health.h"


//#include "miot_model.h"
//#include "mijia_model.h"

/* default group address */
#define MI_DEFAULT_GROUP_ADDR_NUM                10
#define MI_DEFAULT_PRE_SUB_ADDR_NUM              5
/* default ttl number */
#define MI_DEFAULT_TTL                           5
/* network default parameters */
#define MI_NET_RETRANS_COUNT                     7 //8*10ms
#define MI_NET_RETRANS_INTERVAL_STEPS            0 //(n + 1) * 10ms
/* relay default parameters */
#define MI_RELAY_RETRANS_COUNT                   2
#define MI_RELAY_RETRANS_INTERVAL_STEPS          4 //(n + 1) * 10ms
/* network message cache */
#define MI_NMC_SIZE                              96
/* reply protection list size */
#define MI_RPL_SIZE                              32
/* iv update trigger sequence */
#define MI_IV_UPDATE_TRIGGER_SEQUENCE_NUM        0xf00000
/* time of expecting to receive segment acknowledge */
#define MI_TIME_RECV_SEG_ACK                     350
/* time after sending segment acknowledgement */
#define MI_TIME_SEND_SEG_ACK                     300
/* scheduler task number */
#define MI_GAP_SCHED_TASK_NUM                    15
/* relay parallel max number */
#define MI_GAP_SCHED_RELAY_PARALLEL_MAX_NUM      5

#define MI_GATT_TIMEOUT                         20000
#define MI_REGSUCC_TIMEOUT                      5000



/* mi inner message type */
typedef enum
{
    MI_SCHD_PROCESS,
} mi_inner_msg_type_t;

/* mi inner message data */
typedef struct
{
    mi_inner_msg_type_t type;
    uint16_t sub_type;
    union
    {
        uint32_t parm;
        void *pbuf;
    };
} mi_inner_msg_t;

/* app message parameters */
//static uint8_t mi_event;

//extern gapDevDiscReq_t bleMesh_scanparam;
//extern gapAdvertisingParams_t bleMesh_advparam;
//extern UCHAR bleMesh_DiscCancel ;
static uint8_t is_provisioned = false;
//static uint8_t conn_handle = 0xFF;        /* handle of the last opened LE connection */
/* mible state, avoid multiple initialize */
//static bool mible_start = FALSE;
//static bool is_initialized = false;
static bool is_prov_complete = false;
/* connect paramenters */
static uint16_t mible_conn_handle = 0xFFFF;
/* connect timer */
static void *mible_conn_timer = NULL;
extern uint8_t bleMesh_TaskID;
EM_timer_handle prov_thandle;


/* pre subscribe address */
#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL)
static uint16_t pre_sub_addr[] = {0xFE00};
#elif defined(MI_MESH_TEMPLATE_ONE_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
/* low power node */
#if MI_MESH_LOW_POWER_NODE
static uint16_t pre_sub_addr[] = {0xFE01, 0xFE41};
#else
static uint16_t pre_sub_addr[] = {0xFE01};
#endif /* MI_MESH_LOW_POWER_NODE */
#elif defined(MI_MESH_TEMPLATE_FAN)
static uint16_t pre_sub_addr[] = {0xFE03};
#else
//static uint16_t pre_sub_addr[] = {};
#endif /* MI_MESH_TEMPLATE */
//static uint16_t pre_sub_addr_cnt = 0;
extern MS_ACCESS_MODEL_HANDLE   UI_generic_onoff_server_model_handle;
extern mible_status_t mible_record_init(void);

static void process_mesh_node_init_event(void)
{
    mible_mesh_template_map_t node_map[5] = {
        [0] = {
            .siid = 0,
            .piid = 0,
            .model_id = MIBLE_MESH_MIOT_SPEC_SERVER_MODEL,
            .company_id = MIBLE_MESH_COMPANY_ID_XIAOMI,
            .element = 0,
            .appkey_idx = 0,
        },
        [1] = {
            .siid = 0,
            .piid = 0,
            .model_id = MIBLE_MESH_MIJIA_SERVER_MODEL,
            .company_id = MIBLE_MESH_COMPANY_ID_XIAOMI,
            .element = 0,
            .appkey_idx = 0,
        },
#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL) || defined(MI_MESH_TEMPLATE_ONE_KEY_SWITCH) \
|| defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_FAN)
        [2] = {
            .siid = 2,
            .piid = 1,
            .model_id = MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_SERVER,
            .company_id = MIBLE_MESH_COMPANY_ID_SIG,
            .element = 0,
            .appkey_idx = 0,
        },
#if defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
        [3] = {
            .siid = 3,
            .piid = 1,
            .model_id = MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_SERVER,
            .company_id = MIBLE_MESH_COMPANY_ID_SIG,
            .element = 1,
            .appkey_idx = 0,
        },
    #if defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
        [4] = {
            .siid = 4,
            .piid = 1,
            .model_id = MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_SERVER,
            .company_id = MIBLE_MESH_COMPANY_ID_SIG,
            .element = 2,
            .appkey_idx = 0,
        },
    #else
        [4] = {0},
    #endif
#elif defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL)
        [3] = {
            .siid = 2,
            .piid = 2,
            .model_id = MIBLE_MESH_MODEL_ID_LIGHTNESS_SERVER,
            .company_id = MIBLE_MESH_COMPANY_ID_SIG,
            .element = 0,
            .appkey_idx = 0,
        },
    #if defined(MI_MESH_TEMPLATE_LIGHTCTL)
        [4] = {
            .siid = 2,
            .piid = 3,
            .model_id = MIBLE_MESH_MODEL_ID_CTL_TEMPEATURE_SERVER,
            .company_id = MIBLE_MESH_COMPANY_ID_SIG,
            .element = 1,
            .appkey_idx = 0,
        },
    #else
        [4] = {0},
    #endif
#else
        [3] = {0},
        [4] = {0},
#endif
#else
        [2] = {0},
        [3] = {0},
        [4] = {0},
#endif
    };
    mible_mesh_node_init_t node_info;
    node_info.map = (mible_mesh_template_map_t *)&node_map;
    
    node_info.lpn_node = 0;
	MS_NET_ADDR uaddr;
	API_RESULT  retval;
	retval = MS_access_cm_get_primary_unicast_address(&uaddr);
	if (retval == API_SUCCESS)
	{
		if (MS_NET_ADDR_UNASSIGNED != uaddr)
        {
            /* Set Provisioning is not Required */
            is_provisioned = true;
        }
		else
		{
			is_provisioned = false;
		}
	}
	node_info.provisioned = is_provisioned;//(cb_type == PROV_CB_TYPE_PROV) ? 1:0;
	node_info.address = uaddr;
//    node_info.address = mesh_node.unicast_addr;
    node_info.ivi = ms_iv_index.iv_index&0x01;
    //is_provisioned = node_info.provisioned;
    MI_LOG_INFO("node state %d, addr %04x, ivi %08x\n", is_provisioned,
                uaddr, node_info.ivi);
//    MS_private_server_adv_start(0x2);
    mible_mesh_event_callback(MIBLE_MESH_EVENT_DEVICE_INIT_DONE, &node_info);
}

//MS_ACCESS_MODEL_CB         generic_onoff_server_cb;
//MS_GENERIC_SERVER_CB       generic_server_appl_cb;
//static DECL_CONST UINT32 generic_onoff_server_opcode_list[] =
//{
//    MS_ACCESS_GENERIC_ONOFF_SET_UNACKNOWLEDGED_OPCODE,
//    MS_ACCESS_GENERIC_ONOFF_SET_OPCODE,
//    MS_ACCESS_GENERIC_ONOFF_GET_OPCODE,
//};

//API_RESULT MS_generic_server_register
//(
//	UINT16 model_id,
//	MS_ACCESS_MODEL_CB cb,
//	UINT32*  opcodes,
//    MS_ACCESS_ELEMENT_HANDLE element_handle,
//    /* INOUT */ MS_ACCESS_MODEL_HANDLE* model_handle,
//    MS_GENERIC_SERVER_CB appl_cb
//)
//{
//	 API_RESULT retval;
//    MS_ACCESS_NODE_ID        node_id;
//    MS_ACCESS_MODEL          model;
//    /* TBD: Initialize MUTEX and other data structures */
//    /* Using default node ID */
//    node_id = MS_ACCESS_DEFAULT_NODE_ID;
//	 /* Configure Model */
//    model.model_id.id = model_id;
//    model.model_id.type = MS_ACCESS_MODEL_TYPE_SIG;
//    model.elem_handle = element_handle;
//    /* Register Callbacks */
//    model.cb = cb;  //receive cb
//    model.pub_cb = NULL;
//    /* List of Opcodes */
//    model.opcodes = opcodes;
//    model.num_opcodes = sizeof(opcodes) / sizeof(UINT32);
//    retval = MS_access_register_model
//             (
//                 node_id,
//                 &model,
//                 model_handle
//             );
//    /* Save Application Callback */
////    generic_onoff_server_appl_cb = appl_cb;
//    /* TODO: Remove */
////    generic_onoff_server_model_handle = *model_handle;
//	return retval;
//}
API_RESULT UI_app_config_server_callback (
    /* IN */ MS_ACCESS_MODEL_HANDLE*  handle,
    /* IN */ MS_NET_ADDR               saddr,
    /* IN */ MS_NET_ADDR               daddr,
    /* IN */ MS_SUBNET_HANDLE          subnet_handle,
    /* IN */ MS_APPKEY_HANDLE          appkey_handle,
    /* IN */ UINT32                    opcode,
    /* IN */ UCHAR*                    data_parm,
    /* IN */ UINT16                    data_len,
    /* IN */ API_RESULT                retval,
    /* IN */ UINT32                    response_opcode,
    /* IN */ UCHAR*                    response_buffer,
    /* IN */ UINT16                    response_buffer_len)
{
	
//	uint8_t tx_state;
//    UCHAR  proxy_state;
    #ifdef EASY_BOUNDING
    MS_ACCESS_ADDRESS         addr;
    #endif
    MI_LOG_INFO("[APP_CFG_SERV_CB] %04x \n", opcode);
	mible_mesh_event_params_t evt_config_param;
	MS_NET_ADDR uaddr;
	retval = MS_access_cm_get_primary_unicast_address(&uaddr);
	memset(&evt_config_param.config_msg, 0, sizeof(mible_mesh_config_status_t));
	evt_config_param.config_msg.opcode.opcode = opcode;
	evt_config_param.config_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
	evt_config_param.config_msg.meta_data.dst_addr = daddr;
	evt_config_param.config_msg.meta_data.src_addr = saddr;
	evt_config_param.config_msg.meta_data.appkey_index = appkey_handle;
	evt_config_param.config_msg.meta_data.elem_index = daddr - uaddr;
	
    switch (opcode)
    {
    case MS_ACCESS_CONFIG_NODE_RESET_OPCODE:

    case MS_ACCESS_CONFIG_MODEL_SUBSCRIPTION_ADD_OPCODE:

    case MS_ACCESS_CONFIG_MODEL_SUBSCRIPTION_DELETE_OPCODE:

    case MS_ACCESS_CONFIG_NETWORK_TRANSMIT_SET_OPCODE:

    case MS_ACCESS_CONFIG_APPKEY_ADD_OPCODE:

    case MS_ACCESS_CONFIG_MODEL_APP_BIND_OPCODE:
		evt_config_param.config_msg.model_sub_set.elem_addr = daddr;
		evt_config_param.config_msg.model_sub_set.address = uaddr;
		evt_config_param.config_msg.model_sub_set.model_id.model_id = MIBLE_MESH_MODEL_ID_CONFIGURATION_SERVER;
		evt_config_param.config_msg.model_sub_set.model_id.company_id = MIBLE_MESH_COMPANY_ID_SIG;
        break;
	case MS_ACCESS_CONFIG_RELAY_SET_OPCODE:
		evt_config_param.config_msg.relay_set.relay = ms_features|(1<<MS_FEATURE_RELAY);
		evt_config_param.config_msg.relay_set.relay_retrans_cnt = ms_tx_state[MS_RELAY_TX_STATE].tx_count;
        evt_config_param.config_msg.relay_set.relay_retrans_intvlsteps = ms_tx_state[MS_RELAY_TX_STATE].tx_steps;
		
		break;

    default:
        break;
    }
	mible_mesh_event_callback(MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, &evt_config_param);
    return API_SUCCESS;
}

API_RESULT UI_register_foundation_model_servers
(
    MS_ACCESS_ELEMENT_HANDLE element_handle
)
{
    /* Configuration Server */
    MS_ACCESS_MODEL_HANDLE   UI_config_server_model_handle;
    API_RESULT retval;
    #ifdef USE_HEALTH
    /* Health Server */
    MS_ACCESS_MODEL_HANDLE   UI_health_server_model_handle;
    UINT16                       company_id;
    MS_HEALTH_SERVER_SELF_TEST* self_tests;
    UINT32                       num_self_tests;
    #endif
//    MI_LOG_INFO("In Model Server - Foundation Models\n");
    retval = MS_config_server_init(element_handle, &UI_config_server_model_handle);
//    MI_LOG_INFO("Config Model Server Registration Status: 0x%04X\n", retval);
    #ifdef USE_HEALTH
    /* Health Server */
    company_id = MS_DEFAULT_COMPANY_ID;
    self_tests = &UI_health_server_self_tests[0];
    num_self_tests = sizeof(UI_health_server_self_tests)/sizeof(MS_HEALTH_SERVER_SELF_TEST);
    retval = MS_health_server_init
             (
                 element_handle,
                 &UI_health_server_model_handle,
                 company_id,
                 self_tests,
                 num_self_tests,
                 UI_health_server_cb
             );

    if (API_SUCCESS == retval)
    {
        MI_LOG_INFO(
            "Health Server Initialized. Model Handle: 0x%04X\n",
            UI_health_server_model_handle);
    }
    else
    {
        MI_LOG_INFO(
            "[ERR] Sensor Server Initialization Failed. Result: 0x%04X\n",
            retval);
    }

    #endif
    return retval;
}



static int init_models(void)
{
    uint16_t result = 0;

	MS_ACCESS_NODE_ID node_id;
    MS_ACCESS_ELEMENT_DESC   element;
    MS_ACCESS_ELEMENT_HANDLE element_handle;
//	MS_ACCESS_ELEMENT_HANDLE element_handle1;
//	MS_ACCESS_ELEMENT_HANDLE element_handle2;
    API_RESULT retval;
	
	/* Create Node */
    retval = MS_access_create_node(&node_id);

	/* create elements */
	element.loc = 0x0106;
    retval = MS_access_register_element
             (
                 node_id,
                 &element,
                 &element_handle
              );

	if (API_SUCCESS == retval)
    {
        /* Register foundation model servers */
        retval = UI_register_foundation_model_servers(element_handle);
    }

	//model register 
	//generic onoff example
	if (API_SUCCESS == retval)
    {
        /* Register Generic OnOff model server */
        retval = UI_register_generic_onoff_model_server(element_handle);
    }
	APP_config_server_CB_init(UI_app_config_server_callback); //config cb
    return result;
}
void timeout_prov_state_cb (void* args, UINT16 size)
{
    prov_thandle = EM_TIMER_HANDLE_INIT_VAL;
//	LOG("node state\n");
    process_mesh_node_init_event();
}

/**
 *@brief    async method, init mesh stack.
 *          report event: MIBLE_MESH_EVENT_STACK_INIT_DONE, data: NULL.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_init_stack(void)
{
//	MS_ACCESS_NODE_ID node_id;
  	MS_CONFIG* config_ptr;
	API_RESULT retval;
	#ifdef MS_HAVE_DYNAMIC_CONFIG
    MS_CONFIG  config;
    /* Initialize dynamic configuration */
    MS_INIT_CONFIG(config);
    config_ptr = &config;
    #else
    config_ptr = NULL;
	#endif /* MS_HAVE_DYNAMIC_CONFIG */


	/* Initialize OSAL */
    EM_os_init();
    /* Initialize Debug Module */
    EM_debug_init();
    /* Initialize Timer Module */
    EM_timer_init();
    timer_em_init();
    #if defined ( EM_USE_EXT_TIMER )
    EXT_cbtimer_init();
    ext_cbtimer_em_init();
    #endif
	
	/* Initialize utilities */
	nvsto_init(NVS_FLASH_BASE1,NVS_FLASH_BASE2);
    /* Initialize Mesh Stack */
    MS_init(config_ptr);

	/* Register with underlying BLE stack */
    blebrr_register();
    /* Register GATT Bearer Connection/Disconnection Event Hook */
//    blebrr_register_gatt_iface_event_pl(UI_gatt_iface_event_pl_cb);
	/* register element and models */
    init_models();
	UI_model_states_initialization();
	UI_set_brr_scan_rsp_data();
	EM_start_timer (&prov_thandle, 3, timeout_prov_state_cb, NULL, 0);
//	UI_model_states_initialization();
    return 0;
}
/**
 *@brief    deinit mesh stack.
 *          report event: MIBLE_MESH_EVENT_STACK_DEINIT_DONE, data: NULL.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_deinit_stack(void)
{
    return 0;
}

/**
 *@brief    async method, init mesh device
 *          load self info, include unicast address, iv, seq_num, init model;
 *          clear local db, related appkey_list, netkey_list, device_key_list,
 *          we will load latest data for cloud;
 *          report event: MIBLE_MESH_EVENT_DEVICE_INIT_DONE, data: NULL.
 *@param    [in] info : init parameters corresponding to gateway
 *@return   0: success, negetive value: failure
 */
extern void rtk_mesh_stack_start(void);
int mible_mesh_device_init_node(void)
{
    /* send event in process_mesh_node_init_event */
    /** start mesh stack */
    //rtk_mesh_stack_start();
    //prov_params_set(PROV_PARAMS_CALLBACK_FUN, prov_cb, sizeof(prov_cb_pf));
    return 0;
}

/**
 *@brief    set node provsion data.
 *@param    [in] param : prov data include devkey, netkey, netkey idx,
 *          uni addr, iv idx, key flag
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_provsion_data(mible_mesh_provisioning_data_t *param)
{
	PROV_DATA_S*     prov_data;
	memcpy(prov_data->netkey,param->netkey,16);
	prov_data->keyid = param->net_idx;
	prov_data->flags = param->flags;
	prov_data->ivindex = param->iv;
	prov_data->uaddr = param->address;
	return MS_access_cm_set_prov_data(prov_data);
}

/**
 *@brief    mesh provsion done. need update node info and
 *          callback MIBLE_MESH_EVENT_DEVICE_INIT_DONE event
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_provsion_done(void)
{
    is_prov_complete = true;
    mible_timer_start(mible_conn_timer, MI_REGSUCC_TIMEOUT, NULL);
    return 0;
}

/**
 *@brief    reset node, 4.3.2.53 Config Node Reset, Report 4.3.2.54 Config Node Reset Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_config_status_t.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_reset(void)
{
    // erase mesh data
    return MS_common_reset();
}

/**
 *@brief    mesh unprovsion done. need update node info and
 *          callback MIBLE_MESH_EVENT_DEVICE_INIT_DONE event
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_unprovsion_done(void)
{
    return mible_reboot();
}

/**
 *@brief    mesh login done.
 *@return   0: success, negetive value: failure
 */
static void mible_conn_timeout_cb(void *p_context);
int mible_mesh_device_login_done(uint8_t status)
{
    mible_timer_stop(mible_conn_timer);
    if(status){
        MI_LOG_INFO("LOGIN SUCCESS, stop TIMER_ID_CONN_TIMEOUT\n");
    }else{
        MI_LOG_INFO("LOGIN FAIL\n");
        mible_conn_timeout_cb(NULL);
    }
    return 0;
}

/**
 *@brief    set local provisioner network transmit params.
 *@param    [in] count : advertise counter for every adv packet, adv transmit times
 *@param    [in] interval_steps : adv interval = interval_steps*0.625ms
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_network_transmit_param(uint8_t count, uint8_t interval_steps)
{
    MI_LOG_WARNING("[mible_mesh_gateway_set_network_transmit_param] \n");
    // TODO: Store in rom
    ms_tx_state[MS_NETWORK_TX_STATE].tx_count = count & 0x07;
	ms_tx_state[MS_NETWORK_TX_STATE].tx_steps = interval_steps;
	MS_access_ps_store_all_record();
	return 0;
}

/**
 *@brief    set node relay onoff.
 *@param    [in] enabled : 0: relay off, 1: relay on
 *@param    [in] count: Number of relay transmissions beyond the initial one. Range: 0-7
 *@param    [in] interval: Relay retransmit interval steps. 10*(1+steps) milliseconds. Range: 0-31.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_relay(uint8_t enabled, uint8_t count, uint8_t interval)
{
	MS_access_cm_set_features_field(enabled, MS_FEATURE_RELAY);
	ms_tx_state[MS_RELAY_TX_STATE].tx_count = count & 0x07;
	ms_tx_state[MS_RELAY_TX_STATE].tx_steps = interval;
	MS_access_ps_store_all_record();
   	return 0;
}

/**
 *@brief    get node relay state.
 *@param    [out] enabled : 0: relay off, 1: relay on
 *@param    [out] count: Number of relay transmissions beyond the initial one. Range: 0-7
 *@param    [out] interval: Relay retransmit interval steps. 10*(1+steps) milliseconds. Range: 0-31.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_get_relay(uint8_t *enabled, uint8_t *count, uint8_t *step)
{
	*count = ms_tx_state[MS_RELAY_TX_STATE].tx_count;
	*step = ms_tx_state[MS_RELAY_TX_STATE].tx_steps;
	return 0;
}

/**
 *@brief    set node relay onoff.
 *@param    [in] ttl : Time To Live
 *@param    [in] count: Number of net transmissions beyond the initial one. Range: 0-7
 *@param    [in] interval: net retransmit interval steps. 10*(1+steps) milliseconds. Range: 0-31.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_nettx(uint8_t ttl, uint8_t count, uint8_t interval)
{
	API_RESULT retval;
	retval = MS_access_cm_set_default_ttl(ttl);
	if(retval == API_SUCCESS)

	{
		ms_tx_state[MS_NETWORK_TX_STATE].tx_count = count & 0x07;
		ms_tx_state[MS_NETWORK_TX_STATE].tx_steps = interval;
		MS_access_ps_store_all_record();
	}
	return 0;
}

/**
 *@brief    get node relay state.
 *@param    [out] ttl : Time To Live
 *@param    [out] count: Number of net transmissions beyond the initial one. Range: 0-7
 *@param    [out] interval: net retransmit interval steps. 10*(1+steps) milliseconds. Range: 0-31.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_get_nettx(uint8_t *ttl, uint8_t *count, uint8_t *step)
{
	API_RESULT retval;
	retval = MS_access_cm_get_default_ttl(ttl);
	if(retval == API_SUCCESS)
	{
		*count = ms_tx_state[MS_NETWORK_TX_STATE].tx_count;
		*step = ms_tx_state[MS_NETWORK_TX_STATE].tx_steps;
	}
	return 0;
}

/**
 *@brief    set seq number.
 *@param    [in] element : model element
 *@param    [in] seq : current sequence numer
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_seq(uint16_t element, uint32_t seq)
{
	net_seq_number_state.seq_num = seq;
	MS_access_ps_store_all_record();
    return 0;
}

/**
 *@brief    get seq number.
 *@param    [in] element : model element
 *@param    [out] seq : current sequence numer
 *@param    [out] iv_index : current IV Index
 *@param    [out] flags : IV Update Flag
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_get_seq(uint16_t element, uint32_t* seq, uint32_t* iv, uint8_t* flags)
{
	 if(NULL != seq)
    	*seq = net_seq_number_state.seq_num;
     return MS_access_cm_get_iv_index(iv,flags);  
}

int mible_mesh_device_snb_start(bool enable)
{
	if(enable)
	{
		MS_ENABLE_SNB_FEATURE();
		MS_net_start_snb_timer(0);		
	}
	else
	{
		MS_DISABLE_SNB_FEATURE();
        MS_net_stop_snb_timer(0);
	}
    return 0;
}

/**
 *@brief    update iv index, .
 *@param    [in] iv_index : current IV Index
 *@param    [in] flags : contains the Key Refresh Flag and IV Update Flag
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_update_iv_info(uint32_t iv_index, uint8_t flags)
{
//    MI_LOG_WARNING("[mible_mesh_gateway_update_iv_info]  \n");
	return MS_access_cm_set_iv_index(iv_index,flags);
}

/**
 *@brief    add/delete local netkey.
 *@param    [in] op : add or delete
 *@param    [in] netkey_index : key index for netkey
 *@param    [in] netkey : netkey value
 *@param    [in|out] stack_netkey_index : [in] default value: 0xFFFF, [out] stack generates netkey_index
 *          if your stack don't manage netkey_index and stack_netkey_index relationships, update stack_netkey_index.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_netkey(mible_mesh_op_t op, uint16_t netkey_index, uint8_t *netkey)
{
	API_RESULT retval;
	MS_SUBNET_HANDLE subnet;
	UINT32 opcode;
//	UINT32 index, add_index;
//	UCHAR	match_found = MS_FALSE;
	
	//op add/update
	if(op == MIBLE_MESH_OP_ADD)
	{
		opcode = MS_ACCESS_CONFIG_NETKEY_ADD_OPCODE;
		
		retval = MS_access_cm_add_update_netkey(netkey_index,opcode,netkey); 
	}
	if(op == MIBLE_MESH_OP_OVERWRITE)
	{
		opcode = MS_ACCESS_CONFIG_NETKEY_UPDATE_OPCODE;
		
		retval = MS_access_cm_add_update_netkey(netkey_index,opcode,netkey); 
	}
	else if(op == MIBLE_MESH_OP_DELETE) //del
	{
		retval = MS_access_cm_find_subnet(netkey_index,&subnet);
		if(retval == API_SUCCESS)
			retval =  MS_access_cm_delete_netkey(subnet);
		
	}	
	return retval;
}

/**
 *@brief    add/delete local appkey.
 *@param    [in] op : add or delete
 *@param    [in] netkey_index : key index for netkey
 *@param    [in] appkey_index : key index for appkey
 *@param    [in] appkey : appkey value
 *@param    [in|out] stack_appkey_index : [in] default value: 0xFFFF, [out] stack generates appkey_index
 *          if your stack don't manage appkey_index and stack_appkey_index relationships, update stack_appkey_index.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_appkey(mible_mesh_op_t op, uint16_t netkey_index, uint16_t appkey_index, uint8_t * appkey)
{
    MI_LOG_WARNING("[mible_mesh_gateway_set_appkey] \n");
	API_RESULT retval;
	MS_SUBNET_HANDLE subnet;
	retval = MS_access_cm_find_subnet(netkey_index,&subnet);
    if(op == MIBLE_MESH_OP_ADD){
        // add app_key, bind netkey index       
		if(retval == API_SUCCESS)
			return MS_access_cm_add_appkey(subnet,appkey_index,appkey);
    }else{
    	// delete netkey, unbind netkey index
    	if(retval == API_SUCCESS)
    	{
    		 UINT32 opcode;
			 opcode = MS_ACCESS_CONFIG_APPKEY_DELETE_OPCODE;
			 return MS_access_cm_update_delete_appkey(subnet,appkey_index,opcode,appkey);
		}   
    }
}

//int local_model_find(uint16_t elem_index, uint16_t company_id, uint16_t model_id, mesh_model_p *ppmodel)
//{
//    mesh_element_p pelement = mesh_element_get(elem_index);
//    if (NULL == pelement){
//        MI_LOG_ERROR("local_model_find: invalid element index(%d)", elem_index);
//        return -1;
//    }
//    uint32_t model = model_id;
//    if(company_id == 0 || company_id == MIBLE_MESH_COMPANY_ID_SIG){
//        /* sig model */
//        model = MESH_MODEL_TRANSFORM(model);
//    }else if(company_id == MIBLE_MESH_COMPANY_ID_XIAOMI){
//        /* vendor model */
//        model <<= 16;
//        model |= company_id;
//    }else{
//        MI_LOG_ERROR("local_model_find: invalid model id(0x%04x-0x%04x-%d)", model_id, company_id, elem_index);
//        return -2;
//    }
//    
//    mesh_model_p pmodel = mesh_model_get_by_model_id(pelement, model);
//    if (NULL == pmodel)
//    {
//        MI_LOG_ERROR("local_model_find: invalid model(0x%08x-%d)", model, elem_index);
//        return -3;
//    }
//    
//    *ppmodel = pmodel;
//    return 0;
//}

/**
 *@brief    bind/unbind model app.
 *@param    [in] op : bind is MIBLE_MESH_OP_ADD, unbind is MIBLE_MESH_OP_DELETE
 *@param    [in] company_id: company id
 *@param    [in] model_id : model_id
 *@param    [in] appkey_index : key index for appkey
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_model_app(mible_mesh_op_t op, uint16_t elem_index, uint16_t company_id, uint16_t model_id, uint16_t appkey_index)
{
//   API_RESULT retval;
	MS_ACCESS_MODEL_HANDLE model_handle;
	MS_ACCESS_MODEL_ID id;
	id.id = model_id;
	if(company_id == MIBLE_MESH_COMPANY_ID_SIG)
		id.type = MS_ACCESS_MODEL_TYPE_SIG;
	else
		id.type = MS_ACCESS_MODEL_TYPE_VENDOR;
	MS_access_get_model_handle(elem_index,id,&model_handle);
    if(op == MIBLE_MESH_OP_ADD){
        // bind model appkey_index
        return MS_access_bind_model_app(model_handle,appkey_index);
    }
	else
	{
        // unbind model appkey_index
        return MS_access_unbind_model_app(model_handle,appkey_index);
    }
}

int mible_mesh_device_set_presub_address(mible_mesh_op_t op, uint16_t sub_addr)
{
    if(op == MIBLE_MESH_OP_ADD){
        // add subscription
    }else{
        // delete subscription
    }
    return 0;
}

/**
 *@brief    add/delete subscription params.
 *@param    [in] op : add or delete
 *@param    [in] element : model element
 *@param    [in] company_id: company id
 *@param    [in] model_id : model_id
 *@param    [in] sub_addr: subscription address params
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_sub_address(mible_mesh_op_t op, uint16_t element, uint16_t company_id, uint16_t model_id, uint16_t sub_addr)
{
//    API_RESULT retval;
	MS_ACCESS_MODEL_HANDLE model_handle;
	MS_ACCESS_ADDRESS       subp_addr;
	subp_addr.addr = sub_addr;
	MS_ACCESS_MODEL_ID id;
	id.id = model_id;
	if(company_id == MIBLE_MESH_COMPANY_ID_SIG)
		id.type = MS_ACCESS_MODEL_TYPE_SIG;
	else
		id.type = MS_ACCESS_MODEL_TYPE_VENDOR;
	MS_access_get_model_handle(element,id,&model_handle);
    if(op == MIBLE_MESH_OP_ADD)
	{
        // add subscription
        return MS_access_cm_add_model_subscription(model_handle,&subp_addr);
        
    }
	else if(op == MIBLE_MESH_OP_DELETE)
	{
        // delete subscription
        return MS_access_cm_delete_model_subscription(model_handle,&subp_addr);
    }
	else if(op == MIBLE_MESH_OP_DELETE_ALL)
	{
        return MS_access_cm_delete_all_model_subscription(model_handle);
    }
}

/**
 *@brief    get subscription params.
 *@param    [out] *addr : group_address
 *@param    [in] max_count: the count of addr to put the group address
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_get_sub_address(uint16_t *addr, uint16_t max_count)
{
    MS_ACCESS_MODEL_HANDLE model_handle;
	for(model_handle=0; model_handle < MS_CONFIG_LIMITS(MS_ACCESS_MODEL_COUNT);model_handle++)
	{
		MS_access_cm_get_model_subscription_list(model_handle,&max_count,addr);
	}
    return 0;
}

/**
 *@brief    generic message, Mesh model 3.2, 4.2, 5.2, 6.3, or 7 Summary.
 *          report event: MIBLE_MESH_EVENT_GENERIC_OPTION_CB, data: mible_mesh_access_message_t.
 *@param    [in] param : control parameters corresponding to node
 *          according to opcode, generate a mesh message; extral params: ack_opcode, tid, get_or_set.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_generic_control(mible_mesh_access_message_t *param)
{
    //access_send_pdu
	API_RESULT retval;
	UCHAR* pdata;
	MS_SUBNET_HANDLE sub_handle;

	if(param->buf_len > 8)
        memcpy(pdata, param->p_buf, param->buf_len);
    else
        memcpy(pdata, param->buff, param->buf_len);
	retval = MS_access_cm_find_subnet(param->meta_data.netkey_index,&sub_handle);
	
	if(retval == API_SUCCESS)	
		MS_access_send_pdu
		(
			param->meta_data.src_addr,
			param->meta_data.dst_addr,
			sub_handle,
			param->meta_data.appkey_index,
			MI_DEFAULT_TTL, //default ttl set
			param->opcode.opcode,
			pdata,
			param->buf_len,
			MS_FALSE
		);
	
    return 0;
}

#if 0
void mi_inner_msg_handle(uint8_t event)
{
    if (event != mi_event)
    {
        MI_LOG_ERROR("mi_inner_msg_handle: fail, event(%d) is not mesh event(%d)!", event, mi_event);
        return;
    }

    mi_inner_msg_t inner_msg, *pmsg;
    if (FALSE == plt_os_queue_receive(mi_queue_handle, &inner_msg, 0))
    {
        MI_LOG_ERROR("mi_inner_msg_handle: fail to receive msg!");
        return;
    }

    pmsg = &inner_msg;
    switch (pmsg->type)
    {
    case MI_SCHD_PROCESS:
        mi_schd_process();
        mible_mesh_device_main_thread();
        break;
    default:
        MI_LOG_WARNING("mi_inner_msg_handle: fail, mi queue message unknown type: %d!", pmsg->type);
        break;
    }
}

bool mi_inner_msg_send(mi_inner_msg_t *pmsg)
{
    static uint32_t error_num = 0;

    if (!plt_os_queue_send(mi_queue_handle, pmsg, 0))
    {
        if (++error_num % 20 == 0)
        {
            MI_LOG_ERROR("failed to send msg to mi msg queue");
        }
        return FALSE;
    }

    /* send event to notify app task */
    if (!plt_os_queue_send(mi_event_queue_handle, &mi_event, 0))
    {
        if (++error_num % 20 == 0)
        {
            MI_LOG_ERROR("failed to send msg to mi event queue");
        }
        return FALSE;
    }

    return TRUE;
}

void mi_mesh_start(uint8_t event_mi, void *event_queue)
{
    mi_event = event_mi;
    mi_event_queue_handle = event_queue;
    mi_queue_handle = plt_os_queue_create(MI_INNER_MSG_NUM, sizeof(mi_inner_msg_t));
}
#endif
/* RTC parameters */
#define RTC_PRESCALER_VALUE     (32-1)//f = 1000Hz
/* RTC has four comparators.0~3 */
#define RTC_COMP_INDEX          1
#define RTC_INT_CMP_NUM         RTC_INT_CMP1
#define RTC_COMP_VALUE          (10)

/**
  * @brief  Initialize rtc peripheral.
  * @param   No parameter.
  * @return  void
  */
#if 0
void driver_rtc_init(void)
{
    RTC_DeInit();
    RTC_SetPrescaler(RTC_PRESCALER_VALUE);

    RTC_SetComp(RTC_COMP_INDEX, RTC_COMP_VALUE);
    RTC_MaskINTConfig(RTC_INT_CMP_NUM, DISABLE);
    RTC_CompINTConfig(RTC_INT_CMP_NUM, ENABLE);

    /* Config RTC interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    RTC_RunCmd(ENABLE);
}

void RTC_Handler(void)
{
    if (RTC_GetINTStatus(RTC_INT_CMP_NUM) == SET)
    {
        /* Notes: DBG_DIRECT function is only used for debugging demonstrations, not for application projects.*/
        mi_inner_msg_t msg;
        msg.type = MI_SCHD_PROCESS;
        mi_inner_msg_send(&msg);
        //DBG_DIRECT("[main]RTC_Handler: RTC counter current value = %d", RTC_GetCounter());
        RTC_SetComp(RTC_COMP_INDEX, (RTC_GetCounter() + RTC_COMP_VALUE) & 0xffffff);
        RTC_ClearCompINT(RTC_COMP_INDEX);
    }
}

/**
 * @brief initialize mi gatt process task
 */
void mi_gatts_init(void)
{
    driver_rtc_init();
}

void mi_gatts_suspend(void)
{
    //RTC_RunCmd(DISABLE);
    //MI_LOG_INFO("RTC disabled!");
}

void mi_gatts_resume(void)
{
    //RTC_RunCmd(ENABLE);
    //MI_LOG_INFO("RTC enabled!");
}

void mi_startup_delay(void)
{
    if (PROV_NODE == mesh_node.node_state)
    {
        int delay = rand();
        uint32_t real_delay = delay;
        real_delay %= 2000;
        DBG_DIRECT("startup delay: %d ms", real_delay);
        plt_delay_ms(real_delay);
#if (ROM_WATCH_DOG_ENABLE == 1)
        WDG_Restart();
#endif
    }
}
#endif
static void mible_conn_timeout_cb(void *p_context)
{
    MI_LOG_INFO("[mible_conn_timeout_cb]disconnect handle: %04x\r\n", mible_conn_handle);
    if (0xFFFF != mible_conn_handle)
    {
        mible_gap_disconnect(mible_conn_handle);
    }
}

//void mi_handle_gap_msg(T_IO_MSG *pmsg)
//{
//    T_LE_GAP_MSG gap_msg;
//    memcpy(&gap_msg, &pmsg->u.param, sizeof(pmsg->u.param));
//
//    switch (pmsg->subtype)
//    {
//    case GAP_MSG_LE_DEV_STATE_CHANGE:
//        if (!mible_start)
//        {
//            mible_start = TRUE;
//
//            T_GAP_VENDOR_PRIORITY_PARAM pri_param;
//            memset(&pri_param, 0, sizeof(T_GAP_VENDOR_PRIORITY_PARAM));
//            pri_param.set_priority_mode = GAP_VENDOR_SET_PRIORITY;
//            pri_param.link_priority_mode = GAP_VENDOR_SET_ALL_LINK_PRIORITY;
//            pri_param.link_priority_level = GAP_VENDOR_PRIORITY_LEVEL_10;
//            pri_param.scan_priority.set_priority_flag = true;
//            pri_param.scan_priority.priority_level = GAP_VENDOR_PRIORITY_LEVEL_0;
//            pri_param.adv_priority.set_priority_flag = true;
//            pri_param.adv_priority.priority_level = GAP_VENDOR_PRIORITY_LEVEL_5;
//            pri_param.initiate_priority.set_priority_flag = false;
//            pri_param.initiate_priority.priority_level = GAP_VENDOR_RESERVED_PRIORITY;
//            le_vendor_set_priority(&pri_param);
//        
//            //TODO: HAVE_MSC
//            //mi_scheduler_init(MI_SCHEDULER_INTERVAL, mi_schd_event_handler, &lib_cfg);
//            //mible_record_init();
//            mi_gatts_init();
//
//            mible_mesh_event_callback(MIBLE_MESH_EVENT_STACK_INIT_DONE, NULL);
//            if(is_initialized){
//                process_mesh_node_init_event();
//            }
//            
//            mible_systime_utc_set(0);
//        }
//        break;
//    case GAP_MSG_LE_CONN_STATE_CHANGE:
//        {
//            T_GAP_CONN_STATE conn_state = (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state;
//            uint8_t conn_id = gap_msg.msg_data.gap_conn_state_change.conn_id;
//            MI_LOG_DEBUG("mible_handle_gap_msg: conn_id = %d, new_state = %d, cause = %d",
//                         conn_id, conn_state, gap_msg.msg_data.gap_conn_state_change.disc_cause);
//            if (conn_id >= GAP_SCHED_LE_LINK_NUM)
//            {
//                MI_LOG_WARNING("mible_handle_gap_msg: exceed the maximum supported link num %d",
//                               GAP_SCHED_LE_LINK_NUM);
//                break;
//            }
//
//            mible_gap_evt_param_t param;
//            memset(&param, 0, sizeof(param));
//            param.conn_handle = conn_id;
//            if (conn_state == GAP_CONN_STATE_CONNECTED)
//            {
//                //mi_gatts_resume();
//                T_GAP_CONN_INFO conn_info;
//                le_get_conn_info(conn_id, &conn_info);
//                memcpy(param.connect.peer_addr, conn_info.remote_bd, GAP_BD_ADDR_LEN);
//                param.connect.type = (mible_addr_type_t)conn_info.remote_bd_type;
//                param.connect.role = conn_info.role == GAP_LINK_ROLE_MASTER ? MIBLE_GAP_PERIPHERAL :
//                                     MIBLE_GAP_CENTRAL;
//                le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &param.connect.conn_param.min_conn_interval, conn_id);
//                le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &param.connect.conn_param.max_conn_interval, conn_id);
//                le_get_conn_param(GAP_PARAM_CONN_LATENCY, &param.connect.conn_param.slave_latency, conn_id);
//                le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &param.connect.conn_param.conn_sup_timeout, conn_id);
//
//                mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED, &param);
//                if (NULL == mible_conn_timer)
//                {
//                    mible_status_t status = mible_timer_create(&mible_conn_timer, mible_conn_timeout_cb,
//                                                               MIBLE_TIMER_SINGLE_SHOT);
//                    if (MI_SUCCESS != status)
//                    {
//                        MI_LOG_ERROR("mible_conn_timer: fail, timer is not created");
//                        return;
//                    }
//                    else
//                    {
//                        MI_LOG_DEBUG("mible_conn_timer: succ, timer is created");
//                    }
//                }
//                mible_conn_handle = conn_id;
//                mible_timer_start(mible_conn_timer, MI_GATT_TIMEOUT, NULL);
//            }
//            else if (conn_state == GAP_CONN_STATE_DISCONNECTED)
//            {
//                uint16_t disc_cause = gap_msg.msg_data.gap_conn_state_change.disc_cause;
//                if (disc_cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT))
//                {
//                    param.disconnect.reason = CONNECTION_TIMEOUT;
//                }
//                else if (disc_cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
//                {
//                    param.disconnect.reason = REMOTE_USER_TERMINATED;
//                }
//                else if (disc_cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
//                {
//                    param.disconnect.reason = LOCAL_HOST_TERMINATED;
//                }
//
//                mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNECT, &param);
//                mible_conn_handle = 0xFFFF;
//                mible_timer_stop(mible_conn_timer);
//                // prov complete, node init
//                if(is_prov_complete){
//                    is_prov_complete = false;
//                    prov_cb_data_t droppable_data;
//                    droppable_data.pb_generic_cb_type = PB_GENERIC_CB_MSG;
//                    prov_cb(PROV_CB_TYPE_COMPLETE, droppable_data);
//                }
//            }
//        }
//        break;
//    case GAP_MSG_LE_CONN_PARAM_UPDATE:
//        if (GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS == gap_msg.msg_data.gap_conn_param_update.status)
//        {
//            uint8_t conn_id = gap_msg.msg_data.gap_conn_param_update.conn_id;
//            mible_gap_evt_param_t param;
//            memset(&param, 0, sizeof(param));
//            param.conn_handle = conn_id;
//
//            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &param.update_conn.conn_param.min_conn_interval,
//                              conn_id);
//            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &param.update_conn.conn_param.max_conn_interval,
//                              conn_id);
//            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &param.update_conn.conn_param.slave_latency, conn_id);
//            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &param.update_conn.conn_param.conn_sup_timeout, conn_id);
//            mible_gap_event_callback(MIBLE_GAP_EVT_CONN_PARAM_UPDATED, &param);
//        }
//        break;
//    default:
//        break;
//    }
//}

/**
 * @brief get device id
 * @param[in] opcode: command opcode
 * @param[in] pdata: command data
 * @param[in] len: command length
 */
 #if 0
void get_device_id(uint8_t *pdid)
{
#if (HAVE_OTP_PKI)
    uint8_t buf[512];
    int buf_len;
    msc_crt_t crt;

    buf_len = mi_mesh_otp_read(OTP_DEV_CERT, buf, sizeof(buf));
    if (buf_len > 0)
    {
        mi_crypto_crt_parse_der(buf, buf_len, NULL, &crt);
        if (crt.sn.len <= 8)
        {
            for (int i = 0; i < crt.sn.len; i++)
            {
                pdid[crt.sn.len - 1 - i] = crt.sn.p[i];
            }
        }
    }
#endif
}

static int msc_pwr_manage(bool power_stat)
{
    if (power_stat == 1)
    {
        Pad_Config(P0_4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
    else
    {
        Pad_Config(P0_4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    return 0;
}

static const iic_config_t msc_iic_config =
{
    .scl_pin = P0_5,
    .sda_pin = P0_6,
    .freq = IIC_400K,
};

static mible_libs_config_t lib_cfg =
{
    .msc_onoff = msc_pwr_manage,
    .p_msc_iic_config = (void *) &msc_iic_config,
};
#endif
