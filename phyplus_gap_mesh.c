/**
*********************************************************************************************************
*               Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtk_gap.c
* @brief     xiaomi ble gap api
* @details   Gap data types and functions.
* @author    hector_huang
* @date      2018-1-4
* @version   v1.0
* *********************************************************************************************************
*/
#include <stdlib.h>
#include <string.h>
#include <gap.h>
//#include <gap_scan.h>
//#include <gap_adv.h>
//#include <gap_conn_le.h>
//#include <gap_msg.h>
#include "mible_api.h"
#include "mible_port.h"
#include "mible_type.h"
#define MI_LOG_MODULE_NAME "PPLUS_GAP"
#include "mible_log.h"
#include "peripheral.h"
#include "hci.h"

//#include "mesh_api.h"
//#include "rtk_common.h"
extern uint8_t bleMesh_TaskID;

#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL

//typedef struct
//{
//    uint8_t adv_data[31];
//    uint8_t adv_data_len;
//    gap_sched_adv_type_t adv_type;
//    uint32_t adv_interval;
//    plt_timer_t adv_timer;
//    uint8_t adv_interval_switch;
//} gap_adv_ctx_t;

//static gap_adv_ctx_t rtk_adv_ctx;

#if 1
mible_status_t err_code_convert(bStatus_t cause)
{
    mible_status_t status;
//    switch (cause)
//    {
//    case GAP_CAUSE_SUCCESS:
//        status = MI_SUCCESS;
//        break;
//    case GAP_CAUSE_ALREADY_IN_REQ:
//        status = MI_ERR_BUSY;
//        break;
//    case GAP_CAUSE_INVALID_STATE:
//        status = MI_ERR_INVALID_STATE;
//        break;
//    case GAP_CAUSE_INVALID_PARAM:
//        status = MI_ERR_INVALID_PARAM;
//        break;
//    case GAP_CAUSE_NON_CONN:
//        status = MIBLE_ERR_INVALID_CONN_HANDLE;
//        break;
//    case GAP_CAUSE_NOT_FIND_IRK:
//        status = MIBLE_ERR_UNKNOWN;
//        break;
//    case GAP_CAUSE_ERROR_CREDITS:
//        status = MIBLE_ERR_UNKNOWN;
//        break;
//    case GAP_CAUSE_SEND_REQ_FAILED:
//        status = MIBLE_ERR_UNKNOWN;
//        break;
//    case GAP_CAUSE_NO_RESOURCE:
//        status = MI_ERR_RESOURCES;
//        break;
//    case GAP_CAUSE_INVALID_PDU_SIZE:
//        status = MI_ERR_INVALID_LENGTH;
//        break;
//    case GAP_CAUSE_NOT_FIND:
//        status = MI_ERR_NOT_FOUND;
//        break;
//    case GAP_CAUSE_CONN_LIMIT:
//        status = MIBLE_ERR_UNKNOWN;
//        break;
//    case GAP_CAUSE_NO_BOND:
//        status = MIBLE_ERR_UNKNOWN;
//        break;
//    case GAP_CAUSE_ERROR_UNKNOWN:
//        status = MIBLE_ERR_UNKNOWN;
//        break;
//    default:
//        status = MIBLE_ERR_UNKNOWN;
//        break;
//    }

    return status;
}
#endif
mible_status_t mible_gap_address_get(mible_addr_t mac)
{
    return GAPRole_GetParameter(GAPROLE_BD_ADDR, mac);
}

static uint16_t scan_window = 0;
static uint16_t scan_interval = 0;

mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
                                    mible_gap_scan_param_t scan_param)
{ 
	BLE_gap_set_scan_params
    (
        scan_type,
        scan_param.scan_interval,
        scan_param.scan_window,
        0x00
    );
	
   return BLE_gap_set_scan_enable(0x01);
}

mible_status_t mible_gap_scan_param_get(mible_gap_scan_param_t *scan_param)
{
	scan_param->scan_interval = GAP_GetParamValue(TGAP_GEN_DISC_SCAN_INT);
	scan_param->scan_window = GAP_GetParamValue(TGAP_GEN_DISC_SCAN_WIND);
	scan_param->scan_interval = GAP_GetParamValue( TGAP_CONN_SCAN_INT);
    scan_param->scan_window = GAP_GetParamValue( TGAP_CONN_SCAN_WIND);
	scan_param->timeout = GAP_GetParamValue(TGAP_CONN_PARAM_TIMEOUT);
	return MI_SUCCESS;
}

mible_status_t mible_gap_scan_stop(void)
{
	return BLE_gap_set_scan_enable(0x00);
}

mible_status_t mible_gap_adv_send(void)
{
//    mible_gap_adv_data_set(advertData,sizeof( advertData ),blebrr_scanrsp_data,blebrr_scanrsp_datalen);
	//blebrr beacon send
	
    return MI_SUCCESS;
}

//static void rtk_adv_timeout_handler(void *pargs)
//{
//    T_IO_MSG msg;
//    msg.type = MIBLE_API_MSG_TYPE_ADV_TIMEOUT;
//    mible_api_inner_msg_send(&msg);
//    uint32_t adv_interval = rtk_adv_ctx.adv_interval;
//    if (rtk_adv_ctx.adv_interval_switch)
//    {
//        adv_interval += 10;
//    }
//    rtk_adv_ctx.adv_interval_switch ^= 0x01;
//    plt_timer_change_period(rtk_adv_ctx.adv_timer, adv_interval, 0);
//}

mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_param)
{
    /* ser adv type */
	//set adv param
	 BLE_gap_set_adv_params
     (
         p_param->adv_type,
         p_param->adv_interval_min,
         p_param->adv_interval_max,
         0x00
     );
	//start timer
	/*
	to do
	*/
    return BLE_gap_set_adv_enable(0x01);
}

mible_status_t mible_gap_adv_data_set(uint8_t const *p_data,
                                      uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
	//scan rsp data set
	
	BLE_gap_set_advscanrsp_data(FALSE, p_sr_data, srdlen);
	//adv data set
    BLE_gap_set_advscanrsp_data(TRUE, p_data, dlen);

	return MI_SUCCESS;
}

mible_status_t mible_gap_adv_stop(void)
{
	//stop timer
	/*
	to do
	*/
    return BLE_gap_set_adv_enable(0x00);
}

mible_status_t mible_gap_connect(mible_gap_scan_param_t scan_param,
                                 mible_gap_connect_t conn_param)
{
   	gapEstLinkReq_t params;
    params.taskID = bleMesh_TaskID;
    params.highDutyCycle = TRUE;
    params.whiteList = 0x00;
    params.addrTypePeer = 0xFF;
    VOID osal_memcpy( params.peerAddr, conn_param.peer_addr, B_ADDR_LEN );
    return GAP_EstablishLinkReq( &params );
}

mible_status_t mible_gap_disconnect(uint16_t conn_handle)
{
   return GAP_TerminateLinkReq( bleMesh_TaskID, conn_handle, HCI_DISCONNECT_REMOTE_USER_TERM ) ;
}

mible_status_t mible_gap_update_conn_params(uint16_t conn_handle,
                                            mible_gap_conn_param_t conn_params)
{
	gapUpdateLinkParamReq_t pParams;
	pParams.connectionHandle = conn_handle;
	pParams.intervalMin = conn_params.min_conn_interval;
	pParams.intervalMax = conn_params.max_conn_interval;
	pParams.connLatency = conn_params.slave_latency;
	pParams.connTimeout = conn_params.conn_sup_timeout;
	return GAP_UpdateLinkParamReq(&pParams);
}

