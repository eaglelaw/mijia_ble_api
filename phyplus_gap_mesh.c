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
#include "blebrr.h"
#include "l2cap.h"

//#include "mesh_api.h"
//#include "rtk_common.h"
extern uint8_t bleMesh_TaskID;
extern BLEBRR_GAP_ADV_DATA private_beacon_data;
extern gapDevDiscReq_t bleMesh_scanparam;
extern gapAdvertisingParams_t bleMesh_advparam;
extern BLEBRR_Q_ELEMENT blebrr_bcon[BRR_BCON_COUNT];
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
//    mible_status_t status;
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

    return MI_SUCCESS;
}
#endif
mible_status_t mible_gap_address_get(mible_addr_t mac)
{
	MI_LOG_INFO("mible_gap_address_get\n");
    return GAPRole_GetParameter(GAPROLE_BD_ADDR, mac);
}

//static uint16_t scan_window = 0;
//static uint16_t scan_interval = 0;

mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
                                    mible_gap_scan_param_t scan_param)
{ 
	MI_LOG_INFO("mible_gap_scan_start\n");
	//set scan param
	GAP_SetParamValue( TGAP_GEN_DISC_SCAN_WIND, scan_param.scan_window );
    GAP_SetParamValue( TGAP_GEN_DISC_SCAN_INT, scan_param.scan_interval );
    GAP_SetParamValue( TGAP_FILTER_ADV_REPORTS, FALSE);
    GAP_SetParamValue( TGAP_GEN_DISC_SCAN, 5000 );
    GAP_SetParamValue( TGAP_CONN_SCAN_INT,scan_param.scan_interval );
    GAP_SetParamValue( TGAP_CONN_SCAN_WIND,scan_param.scan_window);
    //GAP_SetParamValue( TGAP_LIM_DISC_SCAN, 0xFFFF );
    bleMesh_scanparam.activeScan = scan_type;
    bleMesh_scanparam.mode = DEVDISC_MODE_ALL; //DEVDISC_MODE_GENERAL;
    bleMesh_scanparam.whiteList = 0x00;
    bleMesh_scanparam.taskID = bleMesh_TaskID;
	blebrr_handle_evt_scan_complete(0x1);
	return MI_SUCCESS;
}

mible_status_t mible_gap_scan_param_get(mible_gap_scan_param_t *scan_param)
{
	MI_LOG_INFO("mible_gap_scan_param_get\n");
	//conn state
	scan_param->scan_interval = GAP_GetParamValue(TGAP_GEN_DISC_SCAN_INT);
	scan_param->scan_window = GAP_GetParamValue(TGAP_GEN_DISC_SCAN_WIND);
	scan_param->scan_interval = GAP_GetParamValue( TGAP_CONN_SCAN_INT);
    scan_param->scan_window = GAP_GetParamValue( TGAP_CONN_SCAN_WIND);
	scan_param->timeout = GAP_GetParamValue(TGAP_CONN_PARAM_TIMEOUT);
	return MI_SUCCESS;
}

mible_status_t mible_gap_scan_stop(void)
{
	MI_LOG_INFO("mible_gap_scan_stop\n");
	return MI_SUCCESS;
}

mible_status_t mible_gap_adv_send(void)
{	
	//blebrr beacon send
	MI_LOG_INFO("mible_gap_adv_send\n");
    return MI_SUCCESS;
}

mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_param)
{
    /* ser adv type */
	MI_LOG_INFO("mible_gap_adv_start\n");
	blebrr_bcon[BRR_BCON_TYPE_PRIVATE_BEACON].g_adv_flg = true;
	
	//set adv param
//	GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MIN, p_param->adv_interval_min );
//    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MAX, p_param->adv_interval_max );
//    GAP_SetParamValue( TGAP_GEN_DISC_ADV_MIN, 0);
//    bleMesh_advparam.channelMap = GAP_ADVCHAN_ALL;
//    bleMesh_advparam.eventType = p_param->adv_type;
//    bleMesh_advparam.filterPolicy = 0x00;
//    bleMesh_advparam.initiatorAddrType = 0x00;
//    osal_memset(bleMesh_advparam.initiatorAddr, 0x00, B_ADDR_LEN);
//	blebrr_handle_evt_adv_complete(0x1);
	
    return MI_SUCCESS;
}

mible_status_t mible_gap_adv_data_set(uint8_t const *p_data,
                                      uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
	MI_LOG_INFO("mible_gap_adv_data_set\n");
	//set beacon data
	EM_mem_copy
		(
	       private_beacon_data.data,
	       p_data,
	       dlen
	    );
	blebrr_set_adv_scanrsp_data_pl((UCHAR*)p_sr_data,srdlen);
	MS_private_server_adv_start(0x2);
	return MI_SUCCESS;
}

mible_status_t mible_gap_adv_stop(void)
{
	//stop timer
	MI_LOG_INFO("mible_gap_adv_stop\n");
	blebrr_bcon[BRR_BCON_TYPE_PRIVATE_BEACON].g_adv_flg = false;
	//clear beacon
    return MI_SUCCESS;
}

mible_status_t mible_gap_connect(mible_gap_scan_param_t scan_param,
                                 mible_gap_connect_t conn_param)
{
	MI_LOG_INFO("mible_gap_connect\n");
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
	MI_LOG_INFO("mible_gap_disconnect\n");
   return GAP_TerminateLinkReq( bleMesh_TaskID, conn_handle, HCI_DISCONNECT_REMOTE_USER_TERM ) ;
}

mible_status_t mible_gap_update_conn_params(uint16_t conn_handle,
                                            mible_gap_conn_param_t conn_params)
{
	MI_LOG_INFO("mible_gap_update_conn_params\n");
	l2capParamUpdateReq_t updateReq;
	updateReq.intervalMin = conn_params.min_conn_interval;
	updateReq.intervalMax = conn_params.max_conn_interval;
	updateReq.slaveLatency = conn_params.slave_latency;
	updateReq.timeoutMultiplier = conn_params.conn_sup_timeout;
	return L2CAP_ConnParamUpdateReq( conn_handle, &updateReq, bleMesh_TaskID );
}

