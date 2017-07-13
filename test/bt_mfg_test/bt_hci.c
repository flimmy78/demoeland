/**
 *  UNPUBLISHED PROPRIETARY SOURCE CODE
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  The contents of this file may not be disclosed to third parties, copied or
 *  duplicated in any form, in whole or in part, without the prior written
 *  permission of MXCHIP Corporation.
 *
 */


#include "mico.h"
#include "mico_rtos.h"
#include "bt_bus.h"
#include "bt_hci.h"
#include "bt_hci_interface.h"
#include "bt_transport_driver.h"
#include "bt_transport_thread.h"
#include "bt_firmware.h"
#include "bt_firmware_image.h"

/******************************************************
 *                      Macros
 ******************************************************/

/* Verify if Bluetooth function returns success.
 * Otherwise, returns the error code immediately.
 * Assert in DEBUG build.
 */

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_ALLOCATE_PACKET_ATTEMPT 50

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

extern OSStatus bt_hci_transport_driver_bus_read_handler        ( bt_packet_t** packet );
static OSStatus bt_hci_transport_driver_event_handler           ( bt_transport_driver_event_t event );
static OSStatus bt_hci_transport_thread_received_packet_handler ( bt_packet_t* packet );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static mico_bool_t                      hci_initialised        = MICO_FALSE;
static bt_hci_incoming_packet_handler_t hci_event_handler      = NULL;
static bt_hci_incoming_packet_handler_t hci_acl_packet_handler = NULL;
static bt_hci_incoming_packet_handler_t hci_sco_packet_handler = NULL;
static bt_packet_pool_t                 hci_acl_data_packet_pool;
static bt_packet_pool_t                 hci_command_packet_pool;
static bt_packet_pool_t                 hci_event_packet_pool;
static mico_mutex_t                     hci_mutex;

/******************************************************
 *               Function Definitions
 ******************************************************/

OSStatus bt_hci_init( void )
{
    OSStatus err;

    if ( hci_initialised == MICO_TRUE )
    {
        return kNoErr;
    }

    /* Initialise packet pools */
    err = bt_packet_pool_init( &hci_command_packet_pool, BT_HCI_COMMAND_PACKET_COUNT, BT_HCI_COMMAND_HEADER_SIZE, BT_HCI_COMMAND_DATA_SIZE );
    require_noerr_string( err, exit, "Error initialising HCI command packet pool" );

    err = bt_packet_pool_init( &hci_event_packet_pool, BT_HCI_EVENT_PACKET_COUNT, BT_HCI_EVENT_HEADER_SIZE, BT_HCI_EVENT_DATA_SIZE );
    require_noerr_string( err, exit, "Error initialising HCI event packet pool" );

    err = bt_packet_pool_init( &hci_acl_data_packet_pool, BT_HCI_ACL_PACKET_COUNT, BT_HCI_ACL_HEADER_SIZE, BT_HCI_ACL_DATA_SIZE );
    require_noerr_string( err, exit, "Error initialising HCI ACL data packet pool" );

    err = bt_firmware_download( );
    //err = bt_firmware_download( bt_hci_firmware_image, bt_hci_firmware_size, bt_hci_firmware_version );
    require_noerr_string( err, exit, "Error download HCI firmware" );

    /* Switch to MPAF mode. Initialise MPAF transport driver */
    err = bt_transport_driver_init( bt_hci_transport_driver_event_handler, bt_hci_transport_driver_bus_read_handler );
    require_noerr_string( err, exit, "Error initialising BT transport driver" );

    /* Initialise globals */
    hci_event_handler      = NULL;
    hci_acl_packet_handler = NULL;
    hci_sco_packet_handler = NULL;

    /* Create MPAF command mutex */
    err = mico_rtos_init_mutex( &hci_mutex );
    require_noerr_string( err, exit, "Error creating HCI mutex" );

    /* Initialise BT transport thread */
    err = bt_transport_thread_init( bt_hci_transport_thread_received_packet_handler );
    require_noerr_string( err, exit, "Error initialising BT transport thread" );

    hci_initialised = MICO_TRUE;
exit:
    return err;
}

OSStatus bt_hci_deinit( void )
{
    if ( hci_initialised == MICO_FALSE )
    {
        return kNoErr;
    }

    /* Initialise BT transport thread */
    bt_transport_thread_deinit( );

    /* Deinitialise transport driver */
    bt_transport_driver_deinit( );

    /* Deinit command mutex */
    mico_rtos_deinit_mutex( &hci_mutex );

    /* Deinitialise packet pools */
    bt_packet_pool_deinit( &hci_command_packet_pool );
    bt_packet_pool_deinit( &hci_event_packet_pool );
    bt_packet_pool_deinit( &hci_acl_data_packet_pool );

    /* Initialise globals */
    hci_event_handler      = NULL;
    hci_acl_packet_handler = NULL;
    hci_sco_packet_handler = NULL;
    hci_initialised        = MICO_FALSE;
    return kNoErr;
}

OSStatus bt_hci_register_event_handler( bt_hci_incoming_packet_handler_t event_handler )
{
    hci_event_handler = event_handler;
    return kNoErr;
}

OSStatus bt_hci_register_acl_data_handler( bt_hci_incoming_packet_handler_t acl_data_handler )
{
    hci_acl_packet_handler = acl_data_handler;
    return kNoErr;
}

OSStatus bt_hci_register_sco_data_handler( bt_hci_incoming_packet_handler_t sco_data_handler )
{
    hci_sco_packet_handler = sco_data_handler;
    return kNoErr;
}

OSStatus bt_hci_execute_application_callback( event_handler_t application_callback, void* arg )
{
    return bt_transport_thread_execute_callback( application_callback, arg );
}

OSStatus bt_hci_create_packet( hci_packet_type_t type, bt_packet_t** packet, uint32_t data_size )
{
    OSStatus       result  = kUnknownErr;
    uint32_t       attempt = 0;

    UNUSED_PARAMETER( data_size );

    while( result != kNoErr )
    {
        switch ( type )
        {
            case HCI_COMMAND_PACKET:
            {
                result = bt_packet_pool_allocate_packet( &hci_command_packet_pool, packet );
                break;
            }
            case HCI_EVENT_PACKET:
            {
                result = bt_packet_pool_allocate_packet( &hci_event_packet_pool, packet );
                break;
            }
            case HCI_ACL_DATA_PACKET:
            {
                result = bt_packet_pool_allocate_packet( &hci_acl_data_packet_pool, packet );
                break;
            }
            default:
            {
                return kUnexpectedErr;
            }
        }

        if ( result != kNoErr )
        {
            /* Sleep for 10 milliseconds to let other threads run and release packets */
            mico_rtos_delay_milliseconds( 10 );
            attempt++;
            check_string( attempt < MAX_ALLOCATE_PACKET_ATTEMPT, "Maximum attempt reached! Check for packet leak!" );
            return MICO_BT_PACKET_POOL_FATAL_ERROR;
        }
    }

    if ( result == MICO_BT_SUCCESS )
    {
        *( ( *packet )->packet_start ) = (uint8_t)type;
    }

    return result;
}

OSStatus bt_hci_create_dynamic_packet( hci_packet_type_t type, bt_packet_t** packet, uint32_t header_size, uint32_t data_size )
{
    OSStatus result = bt_packet_pool_dynamic_allocate_packet( packet, header_size, data_size );

    if ( result == MICO_BT_SUCCESS )
    {
        *( ( *packet )->packet_start ) = (uint8_t)type;
    }

    return result;
}

OSStatus bt_hci_delete_packet( bt_packet_t* packet )
{
    return bt_packet_pool_free_packet( packet );
}

OSStatus bt_hci_send_packet( bt_packet_t* packet )
{
    return bt_transport_thread_send_packet( packet );
}

static OSStatus bt_hci_transport_driver_event_handler( bt_transport_driver_event_t event )
{
    if ( event == TRANSPORT_DRIVER_INCOMING_PACKET_READY )
    {
        return bt_transport_thread_notify_packet_received();
    }

    return MICO_BT_ERROR;
}

static OSStatus bt_hci_transport_thread_received_packet_handler( bt_packet_t* packet )
{
    hci_packet_type_t packet_type = (hci_packet_type_t)( *( packet->packet_start ) );

     /* Invoke the appropriate callback */
     switch ( packet_type )
     {
         case HCI_EVENT_PACKET:
         {
            check_string( hci_event_handler != NULL, "hci_event_handler isn't set" );
            hci_event_handler( packet );
            break;
         }
         case HCI_ACL_DATA_PACKET:
         {
            check_string( hci_acl_packet_handler != NULL, "hci_acl_packet_handler isn't set" );
            hci_acl_packet_handler( packet );
            break;
         }
         case HCI_SCO_DATA_PACKET:
         {
            check_string( hci_sco_packet_handler != NULL, "hci_sco_packet_handler isn't set" );
            hci_sco_packet_handler( packet );
            break;
         }
         default:
         {
            bt_hci_delete_packet( packet );
            return MICO_BT_UNKNOWN_PACKET;
         }
     }

     return MICO_BT_SUCCESS;
}
