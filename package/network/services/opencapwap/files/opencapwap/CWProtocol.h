/************************************************************************************************
 * Copyright (c) 2006-2009 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica	*
 *                          Universita' Campus BioMedico - Italy								*
 *																								*
 * This program is free software; you can redistribute it and/or modify it under the terms		*
 * of the GNU General Public License as published by the Free Software Foundation; either		*
 * version 2 of the License, or (at your option) any later version.								*
 *																								*
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY				*
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A				*
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.						*
 *																								*
 * You should have received a copy of the GNU General Public License along with this			*
 * program; if not, write to the:																*
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,							*
 * MA  02111-1307, USA.																			*
 *																								*
 * -------------------------------------------------------------------------------------------- *
 * Project:  Capwap																				*
 *																								*
 * Authors : Ludovico Rossi (ludo@bluepixysw.com)												*  
 *           Del Moro Andrea (andrea_delmoro@libero.it)											*
 *           Giovannini Federica (giovannini.federica@gmail.com)								*
 *           Massimo Vellucci (m.vellucci@unicampus.it)											*
 *           Mauro Bisson (mauro.bis@gmail.com)													*
 *	         Antonio Davoli (antonio.davoli@gmail.com)                                          *
 *           Babylon Tien (sptyan@gmail.com)                                                    *
 ************************************************************************************************/


#ifndef __CAPWAP_CWProtocol_HEADER__
#define __CAPWAP_CWProtocol_HEADER__

//#define CWSetField32(obj, start, val)	((obj)[start/64]) |= ((val) << (start%64))	
//#define CWGetField32(obj, start, len)	(((obj)[start/64]) & ((0xFFFFFFFFFFFFFFFF >> (64-(len))) << (start%64)) ) >> (start%64)

/*_____________________________________________________*/
/*  *******************___MACRO___*******************  */
//#define CWSetField32(obj, start, val)					((obj)[start/32]) |= ((val) << (start%32))	
//#define CWGetField32(obj, start, len)					(((obj)[start/32]) & ((0xFFFFFFFFUL >> (32-(len))) << (start%32)) ) >> (start%32)

#define CWSetField32(src,start,len,val)					((src) |= (((~(0xFFFFFFFF << (len))) & (val)) << (32 - (start) - (len))))
#define CWGetField32(src,start,len)					((~(0xFFFFFFFF<<(len))) & ((src) >> (32 - (start) - (len))))

#define CW_REWIND_BYTES(buf, bytes, type)				(buf) = (type*)(((char*) (buf)) - (bytes))
#define CW_SKIP_BYTES(buf, bytes, type)					(buf) = (type*)(((char*) (buf)) + (bytes))
#define CW_SKIP_BITS(buf, bits, type)					(buf) = (type*)(((char*) (buf)) + ((bits) / 8))
#define CW_BYTES_TO_STORE_BITS(bits)					((((bits) % 8) == 0) ? ((bits) / 8) : (((bits) / 8)+1))

#define		CW_CREATE_PROTOCOL_MESSAGE(mess, size, err)		CW_CREATE_OBJECT_SIZE_ERR(((mess).msg), (size), err);		\
									CW_ZERO_MEMORY(((mess).msg), (size));						\
									(mess).offset = 0;

#define 	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(ar_name, ar_size, on_err) 	do{\
											CW_CREATE_ARRAY_ERR((ar_name), (ar_size), CWProtocolMessage, on_err);\
											int i;\
											for(i=0;i<(ar_size); i++) {\
												(ar_name)[i].msg = NULL;\
												(ar_name)[i].offset = 0; \
											}\
										}while(0)

#define		CW_FREE_PROTOCOL_MESSAGE(mess)				if(&mess) {CW_FREE_OBJECT(((mess).msg));								\
																	(mess).offset = 0;}
															
#define		CWParseMessageElementStart()				int oldOffset;												\
									if(msgPtr == NULL || valPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);	\
									oldOffset = msgPtr->offset
						
#define		CWParseMessageElementEnd()				CWDebugLog(NULL);											\
									return ((msgPtr->offset) - oldOffset) == len ? CW_TRUE :	\
									CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Message Element Malformed");


/*_________________________________________________________*/
/*  *******************___CONSTANTS___*******************  */

// to be defined
#define     MAX_UDP_PACKET_SIZE					65536
#define 	CW_MAX_BYTES_PER_IMAGE_DATA 		1024

#define		CW_CONTROL_PORT						5246
#define		CW_DATA_PORT						5247
#define		CW_PROTOCOL_VERSION					0
#define		CW_IANA_ENTERPRISE_NUMBER				13277	
#define		CW_IANA_ENTERPRISE_NUMBER_VENDOR_IANA			18357
#define 	CW_CONTROL_HEADER_OFFSET_FOR_MSG_ELEMS			3		//Offset "Seq Num" - "Message Elements"
#define		CW_MAX_SEQ_NUM						255
#define 	CW_MAX_FRAGMENT_ID					65535
#define 	CLEAR_DATA						1
#define		DTLS_ENABLED_DATA					2
#define		CW_PACKET_PLAIN						0
#define		CW_PACKET_CRYPT						1
#define 	CW_DATA_MSG_FRAME_TYPE					1
#define		CW_DATA_MSG_STATS_TYPE					2
#define     CW_DATA_MSG_FREQ_STATS_TYPE             3 /* 2009 Update */
#define     CW_IEEE_802_3_FRAME_TYPE				4
#define     CW_DATA_MSG_KEEP_ALIVE_TYPE				5

#define     CW_IEEE_802_11_FRAME_TYPE				6
#ifdef RTK_CAPWAP
#if defined(CONFIG_RTL_92D_SUPPORT)||defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN)
#define		CW_MAX_RADIOS_PER_WTP			2
#else
#define		CW_MAX_RADIOS_PER_WTP			1
#endif
#define		CW_MAX_WLANS_PER_RADIO					5
#define		CW_MAX_WLANS_PER_WTP					10
#define 	CW_MAX_BSS_RTK_SITE_SURVEY				64
#define		CW_MAX_STATIONS_PER_WTP					(CW_MAX_WLANS_PER_WTP * 64)
#define		VENDOR_IANA_ID							48	// Realtek
#else
#define		CW_MAX_RADIOS_PER_WTP					32
#define		CW_MAX_WLANS_PER_RADIO					16
#endif

// Babylon 
#define CW_MAX_STR_BUFFER_SIZE			2048
#define CW_WTP_NAME_MAX_SIZE			512
#define CW_IMAGE_IDENTIFIER_MAX_SIZE	32  // data of message element ImageIdendifier
#define CW_MAX_SSID_SIZE	32
#define CW_MAX_KEY_SIZE		32
#define CW_GRP_TSC_SIZE		6

// <TRANSPORT_HEADER_FIELDS>
// CAPWAP version (currently 0)
#define 	CW_TRANSPORT_HEADER_VERSION_START			0
#define 	CW_TRANSPORT_HEADER_VERSION_LEN				4

// Mauro
#define		CW_TRANSPORT_HEADER_TYPE_START				4
#define		CW_TRANSPORT_HEADER_TYPE_LEN				4

// Radio ID number (for WTPs with multiple radios)
#define 	CW_TRANSPORT_HEADER_RID_START				13
#define 	CW_TRANSPORT_HEADER_RID_LEN				5

// Length of CAPWAP tunnel header in 4 byte words 
#define 	CW_TRANSPORT_HEADER_HLEN_START				8
#define 	CW_TRANSPORT_HEADER_HLEN_LEN				5

// Wireless Binding ID
#define 	CW_TRANSPORT_HEADER_WBID_START				18
#define 	CW_TRANSPORT_HEADER_WBID_LEN				5

// Format of the frame
#define 	CW_TRANSPORT_HEADER_T_START				23
#define 	CW_TRANSPORT_HEADER_T_LEN				1

// Is a fragment?
#define 	CW_TRANSPORT_HEADER_F_START				24
#define 	CW_TRANSPORT_HEADER_F_LEN				1

// Is NOT the last fragment?
#define 	CW_TRANSPORT_HEADER_L_START				25
#define 	CW_TRANSPORT_HEADER_L_LEN				1

// Is the Wireless optional header present?
#define 	CW_TRANSPORT_HEADER_W_START				26
#define 	CW_TRANSPORT_HEADER_W_LEN				1

// Is the Radio MAC Address optional field present?
#define 	CW_TRANSPORT_HEADER_M_START				27
#define 	CW_TRANSPORT_HEADER_M_LEN				1

// Is the message a keep alive?
#define 	CW_TRANSPORT_HEADER_K_START				28
#define 	CW_TRANSPORT_HEADER_K_LEN				1

// Set to 0 in this version of the protocol
#define 	CW_TRANSPORT_HEADER_FLAGS_START				29
#define 	CW_TRANSPORT_HEADER_FLAGS_LEN				3

// ID of the group of fragments
#define 	CW_TRANSPORT_HEADER_FRAGMENT_ID_START			0
#define 	CW_TRANSPORT_HEADER_FRAGMENT_ID_LEN			16

// Position of this fragment in the group 
#define 	CW_TRANSPORT_HEADER_FRAGMENT_OFFSET_START		16
#define 	CW_TRANSPORT_HEADER_FRAGMENT_OFFSET_LEN			13

// Set to 0 in this version of the protocol
#define 	CW_TRANSPORT_HEADER_RESERVED_START			29
#define 	CW_TRANSPORT_HEADER_RESERVED_LEN			3
// </TRANSPORT_HEADER_FIELDS>


// Message Type Values
#define		CW_MSG_TYPE_VALUE_INVALID					0	// Babylon won't be used!!!
#define		CW_MSG_TYPE_VALUE_DISCOVERY_REQUEST			1
#define		CW_MSG_TYPE_VALUE_DISCOVERY_RESPONSE			2
#define		CW_MSG_TYPE_VALUE_JOIN_REQUEST				3
#define		CW_MSG_TYPE_VALUE_JOIN_RESPONSE				4
#define		CW_MSG_TYPE_VALUE_CONFIGURE_REQUEST			5
#define		CW_MSG_TYPE_VALUE_CONFIGURE_RESPONSE			6
#define		CW_MSG_TYPE_VALUE_CONFIGURE_UPDATE_REQUEST		7
#define		CW_MSG_TYPE_VALUE_CONFIGURE_UPDATE_RESPONSE		8
#define 	CW_MSG_TYPE_VALUE_WTP_EVENT_REQUEST			9
#define 	CW_MSG_TYPE_VALUE_WTP_EVENT_RESPONSE			10
#define		CW_MSG_TYPE_VALUE_CHANGE_STATE_EVENT_REQUEST		11
#define		CW_MSG_TYPE_VALUE_CHANGE_STATE_EVENT_RESPONSE		12
#define		CW_MSG_TYPE_VALUE_ECHO_REQUEST				13
#define		CW_MSG_TYPE_VALUE_ECHO_RESPONSE				14
#define		CW_MSG_TYPE_VALUE_IMAGE_DATA_REQUEST			15
#define		CW_MSG_TYPE_VALUE_IMAGE_DATA_RESPONSE			16
#define		CW_MSG_TYPE_VALUE_RESET_REQUEST				17
#define		CW_MSG_TYPE_VALUE_RESET_RESPONSE			18
#define		CW_MSG_TYPE_VALUE_PRIMARY_DISCOVERY_REQUEST		19
#define		CW_MSG_TYPE_VALUE_PRIMARY_DISCOVERY_RESPONSE		20
#define		CW_MSG_TYPE_VALUE_DATA_TRANSFER_REQUEST			21
#define		CW_MSG_TYPE_VALUE_DATA_TRANSFER_RESPONSE		22
#define		CW_MSG_TYPE_VALUE_CLEAR_CONFIGURATION_REQUEST		23
#define		CW_MSG_TYPE_VALUE_CLEAR_CONFIGURATION_RESPONSE		24
#define		CW_MSG_TYPE_VALUE_STATION_CONFIGURATION_REQUEST		25
#define		CW_MSG_TYPE_VALUE_STATION_CONFIGURATION_RESPONSE	26

// IEEE 802.11 Binding Type
#define		CW_MSG_TYPE_VALUE_WLAN_CONFIGURATION_REQUEST		3398913
#define		CW_MSG_TYPE_VALUE_WLAN_CONFIGURATION_RESPONSE		3398914

#ifdef RTK_CAPWAP
#define		CW_MSG_TYPE_VALUE_RTK_WTP_STATUS_REQUEST				(VENDOR_IANA_ID*256+1)
#define		CW_MSG_TYPE_VALUE_RTK_WTP_STATUS_RESPONSE				(VENDOR_IANA_ID*256+2)
#endif

// Message Elements Type Values	
#define 	CW_MSG_ELEMENT_AC_DESCRIPTOR_CW_TYPE				1
#define 	CW_MSG_ELEMENT_AC_IPV4_LIST_CW_TYPE					2
#define 	CW_MSG_ELEMENT_AC_IPV6_LIST_CW_TYPE					3
#define 	CW_MSG_ELEMENT_AC_NAME_CW_TYPE						4
#define 	CW_MSG_ELEMENT_AC_NAME_INDEX_CW_TYPE				5
#define		CW_MSG_ELEMENT_TIMESTAMP_CW_TYPE					6
#define		CW_MSG_ELEMENT_ADD_MAC_ACL_CW_TYPE					7
#define		CW_MSG_ELEMENT_ADD_STATION_CW_TYPE					8
#define		CW_MSG_ELEMENT_ADD_STATIC_MAC_ACL_CW_TYPE			9
#define 	CW_MSG_ELEMENT_CW_CONTROL_IPV4_ADDRESS_CW_TYPE		10
#define 	CW_MSG_ELEMENT_CW_CONTROL_IPV6_ADDRESS_CW_TYPE		11
#define		CW_MSG_ELEMENT_CW_TIMERS_CW_TYPE					12
#define		CW_MSG_ELEMENT_DATA_TRANSFER_DATA_CW_TYPE			13
#define		CW_MSG_ELEMENT_DATA_TRANSFER_MODE_CW_TYPE			14
#define 	CW_MSG_ELEMENT_CW_DECRYPT_ER_REPORT_CW_TYPE			15
#define 	CW_MSG_ELEMENT_CW_DECRYPT_ER_REPORT_PERIOD_CW_TYPE	16
#define 	CW_MSG_ELEMENT_DELETE_MAC_ACL_CW_TYPE				17
#define 	CW_MSG_ELEMENT_DELETE_STATION_CW_TYPE				18
#define 	CW_MSG_ELEMENT_DELETE_STATIC_MAC_ACL_CW_TYPE		19
#define 	CW_MSG_ELEMENT_DISCOVERY_TYPE_CW_TYPE				20
#define 	CW_MSG_ELEMENT_DUPLICATE_IPV4_ADDRESS_CW_TYPE		21
#define 	CW_MSG_ELEMENT_DUPLICATE_IPV6_ADDRESS_CW_TYPE		22
#define 	CW_MSG_ELEMENT_IDLE_TIMEOUT_CW_TYPE					23
#define 	CW_MSG_ELEMENT_IMAGE_DATA_CW_TYPE					24
#define 	CW_MSG_ELEMENT_IMAGE_IDENTIFIER_CW_TYPE				25
#define 	CW_MSG_ELEMENT_IMAGE_INFO_CW_TYPE					26
#define 	CW_MSG_ELEMENT_INITIATED_DOWNLOAD_CW_TYPE			27
#define 	CW_MSG_ELEMENT_LOCATION_DATA_CW_TYPE				28
#define 	CW_MSG_ELEMENT_MAX_MSG_LEN_CW_TYPE					29
#define 	CW_MSG_ELEMENT_WTP_IPV4_ADDRESS_CW_TYPE				30
#define 	CW_MSG_ELEMENT_RADIO_ADMIN_STATE_CW_TYPE			31
#define 	CW_MSG_ELEMENT_RADIO_OPERAT_STATE_CW_TYPE			32
#define 	CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE					33
#define 	CW_MSG_ELEMENT_RETURNED_MSG_ELEM_CW_TYPE			34
#define 	CW_MSG_ELEMENT_SESSION_ID_CW_TYPE					35
#define 	CW_MSG_ELEMENT_STATISTICS_TIMER_CW_TYPE				36
#define 	CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_BW_CW_TYPE		37
#define 	CW_MSG_ELEMENT_WTP_BOARD_DATA_CW_TYPE				38
#define 	CW_MSG_ELEMENT_WTP_DESCRIPTOR_CW_TYPE				39
#define 	CW_MSG_ELEMENT_WTP_FALLBACK_CW_TYPE					40
#define 	CW_MSG_ELEMENT_WTP_FRAME_TUNNEL_MODE_CW_TYPE		41
#define 	CW_MSG_ELEMENT_WTP_MAC_TYPE_CW_TYPE					44
#define 	CW_MSG_ELEMENT_WTP_NAME_CW_TYPE						45
#define 	CW_MSG_ELEMENT_WTP_OPERAT_STATISTICS_CW_TYPE		46
#define 	CW_MSG_ELEMENT_WTP_RADIO_STATISTICS_CW_TYPE			47
#define 	CW_MSG_ELEMENT_WTP_REBOOT_STATISTICS_CW_TYPE		48
#define 	CW_MSG_ELEMENT_WTP_STATIC_IP_CW_TYPE				49
#ifdef RTK_SMART_ROAMING
#define		CW_MSG_ELEMENT_FREE_STATION_CW_TYPE					50
#define		CW_MSG_ELEMENT_SILENT_STATION_CW_TYPE				51
#define		CW_MSG_ELEMENT_UNSILENT_STATION_CW_TYPE				52
#define		CW_MSG_ELEMENT_DOT11V_STATION_CW_TYPE				53
#define		CW_MSG_ELEMENT_DUAL_STATION_CW_TYPE					54
#define		CW_MSG_ELEMENT_NEW_STATION_CW_TYPE					55
#define		CW_MSG_ELEMENT_STATION_FT_OVER_DS_CW_TYPE			56
#define		CW_MSG_ELEMENT_STATION_FT_OVER_AIR_CW_TYPE			57
#endif

// IEEE 802.11 Message Element
#define 	CW_MSG_ELEMENT_IEEE80211_ADD_WLAN_CW_TYPE						1024
#define 	CW_MSG_ELEMENT_IEEE80211_DELETE_WLAN_CW_TYPE					1027
#define 	CW_MSG_ELEMENT_IEEE80211_UPDATE_WLAN_CW_TYPE					1044
#define 	CW_MSG_ELEMENT_IEEE80211_ASSINED_WTP_BSSID_CW_TYPE				1026
#define 	CW_MSG_ELEMENT_IEEE80211_MULTI_DOMAIN_CAPABILITY_CW_TYPE		1032
#define 	CW_MSG_ELEMENT_IEEE80211_SUPPORTED_RATES_CW_TYPE				1040
#define 	CW_MSG_ELEMENT_IEEE80211_WTP_RADIO_INFORMATION_CW_TYPE			1048
#ifdef RTK_SMART_ROAMING
#define		CW_MSG_ELEMENT_IEEE80211_WTP_CONFIGURATION_CW_TYPE				1049
#endif
#define		CW_MSG_ELEMENT_IEEE80211_WTP_QOS_CW_TYPE						1045
#define		BINDING_MSG_ELEMENT_TYPE_OFDM_CONTROL							1033

#ifdef RTK_CAPWAP
// RTK vendor specific
#define		CW_MSG_ELEMENT_RTK_WLAN_CFG_CW_TYPE				65000
#define 	CW_MSG_ELEMENT_RTK_CHANNEL_CW_TYPE				65001
#define 	CW_MSG_ELEMENT_RTK_POWER_SCALE_CW_TYPE			65002
#define		CW_MSG_ELEMENT_RTK_SITE_SURVEY_REQUEST_CW_TYPE	65003  
#define		CW_MSG_ELEMENT_RTK_SITE_SURVEY_CW_TYPE			65004
#define 	CW_MSG_ELEMENT_RTK_STATION_REQUEST_CW_TYPE		65005
#define 	CW_MSG_ELEMENT_RTK_STATION_CW_TYPE				65006
#endif

// CAPWAP Protocol Variables
#ifdef RTK_CAPWAP
#define		CW_MAX_RETRANSMIT_DEFAULT		3
#else
#define		CW_MAX_RETRANSMIT_DEFAULT		5
#endif
#define 	CW_WAIT_JOIN_DEFAULT			60
#define		CW_REPORT_INTERVAL_DEFAULT		120
#define		CW_STATISTIC_TIMER_DEFAULT		120
#define		CW_JOIN_INTERVAL_DEFAULT 	60

#ifdef RTK_CAPWAP
	#define 	CW_CHANGE_STATE_INTERVAL_DEFAULT 60
#else
#ifdef CW_DEBUGGING
	#define		CW_CHANGE_STATE_INTERVAL_DEFAULT 10
#else
	#define		CW_CHANGE_STATE_INTERVAL_DEFAULT 25
#endif
#endif

#define 	CW_RETRANSMIT_INTERVAL_MICROSECOND		500000		//run state request messages
#define		CW_RETRANSMIT_INTERVAL_SECOND			3			//state machine maintenance
#define		CW_ECHO_INTERVAL_DEFAULT				15
#define		CW_DATA_CHANNEL_KEEP_ALIVE_DEFAULT		30
#define		CW_NEIGHBORDEAD_INTERVAL_DEFAULT		(2*(CW_ECHO_INTERVAL_DEFAULT+1))
#define		CW_NEIGHBORDEAD_RESTART_DISCOVERY_DELTA_DEFAULT	((CW_NEIGHBORDEAD_INTERVAL_DEFAULT) + 10)


/*_________________________________________________________*/
/*  *******************___VARIABLES___*******************  */
static const char *rtk_ifname[2][5] = 
{
		{"wlan0", "wlan0-va0", "wlan0-va1", "wlan0-va2", "wlan0-va3"},
		{"wlan1", "wlan1-va0", "wlan1-va1", "wlan1-va2", "wlan1-va3"}
};

/*_____________________________________________________*/
/*  *******************___TYPES___*******************  */
typedef unsigned char CWMACAddress[6];

// Babylon TODO: correct the Result code usage.
typedef enum {
	CW_PROTOCOL_SUCCESS				= 0, //	Success
	CW_PROTOCOL_FAILURE_AC_LIST			= 1, // AC List message MUST be present
	CW_PROTOCOL_SUCCESS_NAT				= 2, // NAT detected
	CW_PROTOCOL_JOIN_FAILURE					= 3, // unspecified
	CW_PROTOCOL_JOIN_FAILURE_RES_DEPLETION		= 4, // Resource Depletion
	CW_PROTOCOL_JOIN_FAILURE_UNKNOWN_SRC		= 5, // Unknown Source
	CW_PROTOCOL_JOIN_FAILURE_INCORRECT_DATA		= 6, // Incorrect Data
	CW_PROTOCOL_JOIN_FAILURE_ID_IN_USE			= 7, // Session ID Alreadyin Use
	CW_PROTOCOL_JOIN_FAILURE_WTP_HW_UNSUPP		= 8, // WTP Hardware not supported
	CW_PROTOCOL_JOIN_FAILURE_BINDING_UNSUPP		= 9, // Binding not supported
	CW_PROTOCOL_RST_FAILURE_UNABLE_TO_RESET		= 10, // Unable to reset
	CW_PROTOCOL_RST_FAILURE_FIRM_WRT_ERROR		= 11, // Firmware write error
	CW_PROTOCOL_CFG_FAILURE_SERVICE_PROVIDED_ANYHOW	= 12, // Unable to apply requested configuration 
	CW_PROTOCOL_CFG_FAILURE_SERVICE_NOT_PROVIDED	= 13, // Unable to apply requested configuration
	CW_PROTOCOL_IMG_FAILURE_INVALID_CHECKSUM		= 14, // Image Data Error: invalid checksum
	CW_PROTOCOL_IMG_FAILURE_INVALID_DATA_LEN		= 15, // Image Data Error: invalid data length
	CW_PROTOCOL_IMG_FAILURE_OTHER_ERROR				= 16, // Image Data Error: other error
	CW_PROTOCOL_IMG_FAILURE_IMAGE_ALREADY_PRESENT	= 17, // Image Data Error: image already present
	CW_PROTOCOL_FAILURE_INVALID_STATE		= 18, // Message unexpected: invalid in current state
	CW_PROTOCOL_FAILURE_UNRECOGNIZED_REQ		= 19, // Message unexpected: unrecognized request
	CW_PROTOCOL_FAILURE_MISSING_MSG_ELEM		= 20, // Failure: missing mandatory message element
	CW_PROTOCOL_FAILURE_UNRECOGNIZED_MSG_ELEM	= 21,  // Failure: unrecognized message element
	CW_PROTOCOL_FAILURE_DATA_TRANSFER_ERROR = 22		// Failure: no information to transfer

} CWProtocolResultCode;

typedef struct {
	uint8_t *msg;
	int offset;
	int data_msgType; 
} CWProtocolMessage;

// Babylon
static inline void CWProtocolMessageFree(CWProtocolMessage *p)
{
	if (p->msg) {
		free(p->msg);
	}
	free(p);
}

#define		SEQUENCE_ARRAY_SIZE					16
#define 	PENDING_MESSAGE_ELEMENT_COUNT		1		//no fragment
#define 	MAX_PENDING_REQUEST_MSGS	10
#define		UNUSED_MSG_TYPE			CW_MSG_TYPE_VALUE_INVALID

typedef struct {
	unsigned char msgType;
	unsigned char seqNum;
	int timer_usec;
	void (*timer_hdl)(void *);
	CWTimerArg timer_arg;
	CWTimerID timer;
	int retransmission;
	CWProtocolMessage *msgElems;
	int fragmentsNum;
} CWPendingRequestMessage;

#include "CWBinding.h"

typedef struct {
	int payloadType;
	int type;
	int isFragment;
	int last;
	int fragmentID;
	int fragmentOffset;
	int keepAlive;
	CWBindingTransportHeaderValues *bindingValuesPtr;
} CWProtocolTransportHeaderValues;

typedef struct {
	unsigned int messageTypeValue;
	unsigned char seqNum;
	unsigned short msgElemsLen;
//	unsigned int timestamp;
} CWControlHeaderValues;

typedef struct {
	uint8_t *data;
	int dataLen;
	CWProtocolTransportHeaderValues transportVal;
} CWProtocolFragment;

typedef struct {
	int vendorIdentifier;
	enum {
		CW_WTP_MODEL_NUMBER	= 0,
		CW_WTP_SERIAL_NUMBER	= 1,
		CW_BOARD_ID		= 2,
		CW_BOARD_REVISION	= 3,

		CW_WTP_HARDWARE_VERSION	= 0,
		CW_WTP_SOFTWARE_VERSION	= 1,
		CW_BOOT_VERSION		= 2
	} type;
	int length;
	unsigned char *valuePtr;
} CWWTPVendorInfoValues;

typedef struct  {
	int vendorInfosCount;
	CWWTPVendorInfoValues *vendorInfos;
} CWWTPVendorInfos;

typedef struct {
	int maxRadios;
	int radiosInUse;
	int encCapabilities;
	CWWTPVendorInfos vendorInfos;
} CWWTPDescriptor;

typedef enum {
	CW_RESERVE = 1,
	CW_LOCAL_BRIDGING = 2,
	CW_802_DOT_3_BRIDGING = 4,
	CW_NATIVE_BRIDGING = 8,
	CW_ALL_ENC = 15
} CWframeTunnelMode;

typedef enum {
	CW_LOCAL_MAC = 0,
	CW_SPLIT_MAC = 1,
	CW_BOTH = 2
} CWMACType;

typedef struct {
	enum {
		CW_MSG_ELEMENT_DISCOVERY_TYPE_BROADCAST = 0,
		CW_MSG_ELEMENT_DISCOVERY_TYPE_CONFIGURED = 1
	} type;
	CWWTPVendorInfos WTPBoardData;
	CWWTPDescriptor WTPDescriptor;
	CWframeTunnelMode frameTunnelMode;
	CWMACType MACType;

} CWDiscoveryRequestValues;

typedef enum {
	CW_X509_CERTIFICATE = 1,
	CW_PRESHARED = 0
} CWAuthSecurity;

typedef struct {
	CWNetworkLev4Address addr;
	struct sockaddr_in addrIPv4;

	int WTPCount;
} CWProtocolNetworkInterface;

typedef struct {
	int WTPCount;
	struct sockaddr_in addr;
} CWProtocolIPv4NetworkInterface;

typedef struct {
	int WTPCount;
	struct sockaddr_in6 addr;
} CWProtocolIPv6NetworkInterface;

typedef struct {
	int vendorIdentifier;
	enum {
		CW_AC_HARDWARE_VERSION	= 4,
		CW_AC_SOFTWARE_VERSION	= 5
	} type;
	int length;
	int *valuePtr;
} CWACVendorInfoValues;

typedef struct  {
	int vendorInfosCount;
	CWACVendorInfoValues *vendorInfos;
} CWACVendorInfos;

typedef struct {
	int rebootCount;
	int ACInitiatedCount;
	int linkFailurerCount;
	int SWFailureCount;
	int HWFailuireCount;
	int otherFailureCount;
	int unknownFailureCount;
	enum {
		NOT_SUPPORTED=0,
		AC_INITIATED=1,
		LINK_FAILURE=2,
		SW_FAILURE=3,
		HD_FAILURE=4,
		OTHER_FAILURE=5,
		UNKNOWN=255
	}lastFailureType;
}WTPRebootStatisticsInfo;

typedef struct
{
	int radioID;
	int reportInterval;
}WTPDecryptErrorReportValues;

typedef struct
{
	int radiosCount;
	WTPDecryptErrorReportValues *radios;
}WTPDecryptErrorReport;

typedef struct {
	int index;
	char *ACName;
}CWACNameWithIndexValues;

typedef struct {
	int count;
	CWACNameWithIndexValues *ACNameIndex;
}CWACNamesWithIndex;

typedef enum {
	CW_802_DOT_11b = 0x01,
	CW_802_DOT_11a = 0x02,
	CW_802_DOT_11g = 0x04,
	CW_802_DOT_11n = 0x08,
	CW_802_DOT_11ac = 0x10,
	CW_ALL_RADIO_TYPES = 0x1F
} CWradioType;

typedef struct {
	int ID;
	CWradioType type;
} CWRadioInformationValues;

typedef struct {
	int radiosCount;
	CWRadioInformationValues radios[CW_MAX_RADIOS_PER_WTP];
} CWRadiosInformation;

#ifdef RTK_SMART_ROAMING
typedef struct{
	int ID;
	unsigned char mac_addr[6];
}CWWTPWlanConfigurationValues;
typedef struct{
	int wlan_num;
	CWWTPWlanConfigurationValues wlan[CW_MAX_RADIOS_PER_WTP];
}CWWTPWlanConfiguration;
#endif

typedef enum {
	CW_ENABLED = 1,
	CW_DISABLED = 2
} CWstate;

typedef enum {
	AD_NORMAL = 1,
	AD_RADIO_FAILURE = 2,
	AD_SOFTWARE_FAILURE = 3,
	AD_RADAR_DETECTION = 4
} CWAdminCause;

typedef enum {
	OP_NORMAL = 0,
	OP_RADIO_FAILURE = 1,
	OP_SOFTWARE_FAILURE = 2,
	OP_ADMINISTRATIVELY_SET = 3
} CWOperationalCause;

typedef enum {
	CW_IMAGE_DATA_TYPE_DATA = 1,
	CW_IMAGE_DATA_TYPE_EOF = 2,
	CW_IMAGE_DATA_TYPE_ERROR_OCCURRED = 5
} CWImageDataType;


typedef struct {
	int ID;
	CWstate state;
	CWAdminCause cause;
} CWRadioAdminInfoValues;

typedef struct {
	int radiosCount;
	CWRadioAdminInfoValues *radios;
} CWRadiosAdminInfo;

typedef struct {
	int ID;
	CWstate state;
	CWOperationalCause cause;
} CWRadioOperationalInfoValues;

typedef struct {
	int radiosCount;
	CWRadioOperationalInfoValues *radios;
} CWRadiosOperationalInfo;

typedef struct {
	int ID;
	unsigned char numEntries;
	unsigned char length;
	CWMACAddress *decryptErrorMACAddressList;
} CWDecryptErrorReportValues;

typedef struct {
	int radiosCount;
	CWDecryptErrorReportValues *radios;
} CWDecryptErrorReportInfo;

typedef struct {
	enum {
		STATISTICS_NOT_SUPPORTED=0,
		SW_FAILURE_TYPE=1,
		HD_FAILURE_TYPE=2,
		OTHER_TYPES=3,
		UNKNOWN_TYPE=255
	}lastFailureType;
	short int resetCount;
	short int SWFailureCount;
	short int HWFailuireCount;
	short int otherFailureCount;
	short int unknownFailureCount;
	short int configUpdateCount;
	short int channelChangeCount;
	short int bandChangeCount;
	short int currentNoiseFloor;
}WTPRadioStatisticsInfo;

typedef struct {
	unsigned int radioID;
	//Station Mac Address List

	CWList decryptErrorMACAddressList;

	unsigned int reportInterval;
	
	CWstate adminState;
	CWAdminCause adminCause;

	CWstate operationalState;
	CWOperationalCause operationalCause;

	unsigned int TxQueueLevel;
	unsigned int wirelessLinkFramesPerSec;

	WTPRadioStatisticsInfo statistics;	
#ifndef RTK_CAPWAP	
	void* bindingValuesPtr;
#endif
} CWWTPRadioInfoValues;

typedef struct {
	int radioCount;
	CWWTPRadioInfoValues *radiosInfo;
} CWWTPRadiosInfo;

typedef struct {
	uint32_t fileSize;
	uint8_t hash[16];
} CWProtocolImageInformationValues;


typedef struct {
	int vendor_id;
	char imageIdentifier[CW_IMAGE_IDENTIFIER_MAX_SIZE];
} CWProtocolImageIdentifierValues;


/*Update 2009:
	Helper structure to keep track of 
	requested UCI commands (via Vendor specific
	message)*/
typedef struct {
	unsigned short vendorPayloadType;
	void *payload;
} CWProtocolVendorSpecificValues;

#ifdef KM_BY_AC
typedef struct 
{
	unsigned char radio_id;
	unsigned char wlan_id;

	union {
		struct {
			unsigned short ess: 1; // must 1;
			unsigned short ibss: 1; // must 0;
			unsigned short cf_pollable: 1;
			unsigned short cf_poll_req: 1;
			unsigned short privacy: 1;
			unsigned short short_preamble: 1;
			unsigned short PBCC: 1;
			unsigned short channel_agility: 1; /* whether WTP is capable of supporting the HR/DSSS */
	
			unsigned short spectrum_management: 1;	/* MIB: dot11SPectrumManagementRequired */
			unsigned short Qos :1;
			unsigned short short_slot_time: 1;
			unsigned short apsd: 1;	/* MIB: dot11APSDOptionImplemented */
			unsigned short rsv: 1;
			unsigned short osss_ofdm: 1;
			unsigned short delayed_block_ack: 1;	/* MIB: dot11DeleyedBlockAckOptionImplemented */
			unsigned short immediate_block_ack: 1; /* MIB: dot11ImmediateBlockAckOptionImplemeted */
		}s;
		unsigned short u16;
	} capability;
	unsigned char key_index;
	unsigned char key_status; 
			/*RFC5416: 	0: key is the group key, for only multicast & broadcast. (sta keys are in station configuration)
					1: key is the shared wep key (for both unicast & multicast)
					2: AC will begine rekeying the GTK with the STAs in the BSS.
					3: AC has completed rekeying the GTK, and broadcst packets no longer need to be duplicated and transmitted with both GTKs. */
		#define CW_WLAN_KEY_STATUS_GROUP		0
		#define CW_WLAN_KEY_STATUS_SHARED_WEP		1
		#define CW_WLAN_KEY_STATUS_BEGINE_REKEY		2
		#define CW_WLAN_KEY_STATUS_COMPLETE_REKEY	3

	unsigned short key_len;
	unsigned char key[CW_MAX_KEY_SIZE];
	unsigned char group_TSC [CW_GRP_TSC_SIZE]; /* used for multicast/broadccast */
	unsigned char non_WMM_sta_qos;
		#define CW_QOS_CLASS_ID_BEST_EFFORT	0
		#define CW_QOS_CLASS_ID_VIDEO		1
		#define CW_QOS_CLASS_ID_VOICE		2
		#define CW_QOS_CLASS_ID_BACKGROUND	3
	unsigned char auth_type; 
		#define CW_AUTH_TYPE_OPEN_SYSTEM	0
		#define CW_AUTH_TYPE_PSK		1
	unsigned char mac_mode;
		/* defined in CWProtocol.h
			typedef enum {
				CW_LOCAL_MAC = 0,
				CW_SPLIT_MAC = 1,
				CW_BOTH = 2
			} CWMACType;
		  */
	unsigned char tunnel_mode;
		#define CW_TUNNEL_MODE_LOCAL_BRIDGING	0	
		#define CW_TUNNEL_MODE_802_3		1
		#define CW_TUNNEL_MODE_802_11		2
	CWBool	suppress_ssid;
	char ssid[CW_MAX_SSID_SIZE];
}CWProtocolAddWLANValues;


typedef struct 
{
	unsigned char radio_id;
	unsigned char wlan_id;
} CWProtocolDeleteWLANValues;

typedef struct
{
	unsigned char radio_id;
	unsigned char wlan_id;
} CWProtocolUpdateWLANValues;

typedef enum {
	CW_WLANCFG_TYPE_INVALID = 0,
	CW_WLANCFG_TYPE_ADD = 1,
	CW_WLANCFG_TYPE_DEL = 2,
	CW_WLANCFG_TYPE_UPDATE = 3
} CWWlanCfgType; 	


typedef struct
{
	CWWlanCfgType wlancfg_type; 	
	union {
		CWProtocolAddWLANValues *addWlanCfg;
		CWProtocolDeleteWLANValues *deleteWlanCfg;
		CWProtocolUpdateWLANValues *updateWlanCfg;
	} wlan_cfg;
} CWProtocolWlanConfigurationRequestValues;

typedef struct
{
	CWWlanCfgType wlancfg_type; 	
	CWProtocolResultCode result_code;

	// The following values only exist when result_code is SUCCESS.
	unsigned char radio_id;
	unsigned char wlan_id;
	unsigned char bssid[6];
} CWProtocolWlanConfigurationResponseValues;

#else

typedef enum {
		WLAN_CFG_KEY_TYPE_NONE = 0,
		WLAN_CFG_KEY_TYPE_SHARED_WEP40,
		WLAN_CFG_KEY_TYPE_SHARED_WEP104,
		WLAN_CFG_KEY_TYPE_SHARED_WPA_AES,
		WLAN_CFG_KEY_TYPE_SHARED_WPA_TKIP,
		WLAN_CFG_KEY_TYPE_SHARED_WPA2_AES,
		WLAN_CFG_KEY_TYPE_SHARED_WPA2_TKIP,
} rtk_wlan_key_type_t; 

typedef enum {
	WLAN_CFG_PSK_FORMAT_PASSPHRASE = 0,
	WLAN_CFG_PSK_FORMAT_HEX = 1,
} rtk_psk_format_t;

typedef struct {
	uint8_t radio_id;
	uint8_t wlan_id;
	uint8_t bssid[6];
	char ssid[CW_MAX_SSID_SIZE+1];

	rtk_wlan_key_type_t key_type;
	rtk_psk_format_t psk_format;
	char key[CW_MAX_KEY_SIZE*2+1];
		// MUST be string (0 ended)
		// for wep40: must be 5 bytes(for passphrase) or 10 bytes(for hex)
		// for wep104: must be 13 bytes or 26 bytes
		// for wpa or wpa2:
		//     if PASSPHRASE, must 8<=len<64, any words
		//     if HEX, must 64 bytes, only hex character
} CWProtocolRtkWlanConfigValues;
#endif

#ifdef RTK_CAPWAP
typedef struct {
	uint8_t radio_id;
	uint8_t channel;
}CWProtocolRtkChannelValues;

typedef enum {
	POWER_SCALE_100 = 0,
	POWER_SCALE_70 = 1,
	POWER_SCALE_50 = 2,
	POWER_SCALE_35 = 3,
	POWER_SCALE_15 = 4,
	MIN_POWER_SCALE_ENUM_VALUE = 0,
	MAX_POWER_SCALE_ENUM_VALUE = 4,
} rtk_power_scale_t;

typedef struct {
	uint8_t radio_id;
	rtk_power_scale_t power_scale;
}CWProtocolRtkPowerScaleValues;

typedef enum {
	CW_BSS_TYPE_AP		=	0x10,
	CW_BSS_TYPE_ADHOC	=	0x20,
} rtk_bss_type_t;

typedef struct {
	unsigned char bssid[6];	
	char ssid[CW_MAX_SSID_SIZE];
	rtk_bss_type_t bss_type;
	unsigned char channel;
	unsigned char signal_strength;
	CWradioType radio_type;
} rtk_bss_t;

#pragma pack(1)
// this struct is used for CAPWAP and WUM both.
typedef struct {
	unsigned char radioID;
	unsigned char numBss;
	rtk_bss_t bssInfo[CW_MAX_BSS_RTK_SITE_SURVEY];
} CWRtkSiteSurveyValues;
#define CW_RTK_SITE_SURVEY_VALUES_USED_BYTES(s) ( sizeof((s).radioID) + sizeof((s).numBss) + sizeof(rtk_bss_t)*(s).numBss )
#pragma pack(0)

typedef struct {
	unsigned char radio_id;
	unsigned char wlan_id;
	unsigned char mac_addr[6];	
} CWRtkStationValues;

typedef struct {	
	CWProtocolResultCode resultCode;
	CWBool siteSurveyResponsed;	// CW_MSG_ELEMENT_RTK_SITE_SURVEY_REQUEST_CW_TYPE exist
	int numSiteSurvey;
	CWRtkSiteSurveyValues siteSurvey[CW_MAX_RADIOS_PER_WTP];

	CWBool rtkStationsResponsed;
	int numRtkStations;
	CWRtkStationValues rtkStations[CW_MAX_STATIONS_PER_WTP+1];
} CWProtocolRtkWTPStatusResponseValues;

#endif

/*---------------------------*/

#ifdef RTK_SMART_ROAMING
typedef struct{
	uint8_t radio_id;
	uint8_t mac_addr[6];
}CWProtocolRequestStationValues;

typedef struct {
	uint8_t mac_addr[6];
	uint8_t radio_id;
	uint8_t ch_util;
	uint8_t rssi;
	uint8_t data_rate;
	uint32_t link_time;
}CWProtocolAddStationValues;

typedef struct{
	uint8_t mac_addr[6];
	uint8_t radio_id;
	uint8_t rssi;
	uint8_t ch_util;
} CWProtocolUpdateStationValues;

typedef struct{
	uint32_t wtp_index;
	uint8_t ch_util;
	uint8_t rcpi;
}Dot11kValues;

typedef struct{
	uint8_t flag;		//10 - 11k; 11 - 11k&11v
	uint8_t mac_addr[6];
	uint8_t radio_id;
	Dot11kValues Dot11k[STA_MAX_11K_NUM];
}CWProtocolUpdateStationDot11kValues;
#endif

typedef struct {
	int type;
	union {
		int int32;
		char str[CW_MAX_STR_BUFFER_SIZE];
#ifdef RTK_SMART_ROAMING
		CWProtocolRequestStationValues del_sta;
#endif
	} value;
} CWMsgElemData;


#include "CWList.h"

/*__________________________________________________________*/
/*  *******************___PROTOTYPES___*******************  */

__inline__ unsigned int CWGetSeqNum(); // provided by the user of CWProtocol lib
__inline__ int CWGetFragmentID(); // provided by the user of CWProtocol lib

void CWWTPResetRadioStatistics(WTPRadioStatisticsInfo *radioStatistics);

void CWProtocolDestroyMsgElemData(void *f);
void CWFreeMessageFragments(CWProtocolMessage* messages, int fragmentsNum);
#ifdef KM_BY_AC
void CWProtocolAddWLANValueFree(CWProtocolAddWLANValues *pAddWlan);
void CWProtocolAddWLANVlauePrint(const CWProtocolAddWLANValues *pAddWlan);	
#endif
void CWProtocolStore8(CWProtocolMessage *msgPtr, unsigned char val);
void CWProtocolStore16(CWProtocolMessage *msgPtr, unsigned short val);
void CWProtocolStore32(CWProtocolMessage *msgPtr, unsigned int val);
void CWProtocolStoreStr(CWProtocolMessage *msgPtr, const char *str);
void CWProtocolStoreMessage(CWProtocolMessage *msgPtr, CWProtocolMessage *msgToStorePtr);
void CWProtocolStoreRawBytes(CWProtocolMessage *msgPtr, const void *bytes, int len);

unsigned char CWProtocolRetrieve8(CWProtocolMessage *msgPtr);
unsigned short CWProtocolRetrieve16(CWProtocolMessage *msgPtr);
unsigned int CWProtocolRetrieve32(CWProtocolMessage *msgPtr);
char *CWProtocolRetrieveStr(CWProtocolMessage *msgPtr, int len);
void *CWProtocolRetrieveRawBytes(CWProtocolMessage *msgPtr, int len);
void CWProtocolCopyRawBytes(void *buf, CWProtocolMessage *msgPtr, int len);
void *CWProtocolPointRawBytes(CWProtocolMessage *msgPtr, int len);
void CWProtocolSkipElement(CWProtocolMessage *msgPtr, int len);

CWBool CWProtocolParseFragment(unsigned char *buf, int readBytes, CWList *fragmentsListPtr, CWProtocolMessage *reassembledMsg, CWBool *dataFlag, char *RadioMAC);
void CWProtocolDestroyFragment(void *f);

CWBool CWParseTransportHeader(CWProtocolMessage *msgPtr, CWProtocolTransportHeaderValues *valuesPtr, CWBool *dataFlag, char *RadioMAC);
CWBool CWParseControlHeader(CWProtocolMessage *msgPtr, CWControlHeaderValues *valPtr);
CWBool CWParseFormatMsgElem(CWProtocolMessage *completeMsg,unsigned short int *type,unsigned short int *len);

CWBool CWAssembleTransportHeader(CWProtocolMessage *transportHdrPtr, CWProtocolTransportHeaderValues *valuesPtr);
CWBool CWAssembleTransportHeaderKeepAliveData(CWProtocolMessage *transportHdrPtr, CWProtocolTransportHeaderValues *valuesPtr, int keepAlive);
CWBool CWAssembleControlHeader(CWProtocolMessage *controlHdrPtr, CWControlHeaderValues *valPtr);
CWBool CWAssembleMessage(CWProtocolMessage **completeMsgPtr, int *fragmentsNumPtr, int PMTU, int seqNum, int msgTypeValue, CWProtocolMessage *msgElems, const int msgElemNum, CWProtocolMessage *msgElemsBinding, const int msgElemBindingNum, int is_crypted);
CWBool CWAssembleMsgElem(CWProtocolMessage *msgPtr, unsigned int type);
CWBool CWAssembleUnrecognizedMessageResponse(CWProtocolMessage **messagesPtr, int *fragmentsNumPtr, int PMTU, int seqNum, int msgType);

CWBool CWAssembleMsgElemRadioAdminState(CWProtocolMessage *msgPtr);			//29
CWBool CWAssembleMsgElemRadioOperationalState(int radioID, CWProtocolMessage *msgPtr);	//30
CWBool CWAssembleMsgElemResultCode(CWProtocolMessage *msgPtr,CWProtocolResultCode code);//31
CWBool CWAssembleMsgElemSessionID(CWProtocolMessage *msgPtr, unsigned char *sessionID);		//32
CWBool CWAssembleMsgElemImageIdentifier(CWProtocolMessage *msgPtr, int vendor_indentifier, const char *version); // 25
#ifdef RTK_SMART_ROAMING
CWBool CWAssembleMsgElemDeleteStation(int radioID,CWProtocolMessage *msgPtr,unsigned char* StationMacAddr);
CWBool CWAssmebleMsgElemFreeStation(int radioID,CWProtocolMessage *msgPtr,unsigned char *StationMacAddr);
CWBool CWAssembleMsgElemSilentStation(int radioID,CWProtocolMessage *msgPtr,unsigned char* StationMacAddr);
CWBool CWAssembleMsgElemUnsilentStation(int radioID,CWProtocolMessage *msgPtr,unsigned char* StationMacAddr);
CWBool CWAssembleMsgElemDot11vStation(int radioID,CWProtocolMessage *msgPtr,unsigned char* StationMacAddr);
CWBool CWAssembleMsgElemDualStation(int radioID,CWProtocolMessage *msgPtr,unsigned char *StationMacAddr);
CWBool CWAssembleMsgElemAddStation(CWProtocolMessage *msgPtr, int len, unsigned char *str, int type);
#endif
#ifdef RTK_CAPWAP
CWBool CWAssembleMsgElemRtkChannel(CWProtocolMessage *msgPtr, const CWProtocolRtkChannelValues *valPtr);
CWBool CWAssembleMsgElemRtkPowerScale(CWProtocolMessage *msgPtr, const CWProtocolRtkPowerScaleValues *valPtr);
CWBool CWAssembleMsgElemRtkSiteSurveyRequest(CWProtocolMessage *msgPtr);
CWBool CWAssembleMsgElemRtkStationRequest(CWProtocolMessage *msgPtr);

#ifndef KM_BY_AC
CWBool CWAssembleMsgElemRtkWlanConfiguration(CWProtocolMessage *msgPtr, const CWProtocolRtkWlanConfigValues *valPtr);
#endif
#endif


CWBool CWParseMsgElemACName(CWProtocolMessage *msgPtr, int len, char **valPtr);  // 4
CWBool CWParseMsgElemWTPRadioOperationalState (CWProtocolMessage *msgPtr, int len, CWRadioOperationalInfoValues *valPtr);	//30
CWBool CWParseMsgElemResultCode(CWProtocolMessage *msgPtr, int len, CWProtocolResultCode *valPtr);			//31
unsigned char *CWParseMsgElemSessionID(CWProtocolMessage *msgPtr, int len);
CWBool CWParseMsgElemWTPRadioInformation(CWProtocolMessage *msgPtr, int len, unsigned char *valPtr);	//1048
#ifdef RTK_SMART_ROAMING
CWBool CWParseMsgElemWTPConfiguration(CWProtocolMessage *msgPtr, int len, CWWTPWlanConfiguration *valPtr);
CWBool CWParseMsgElemDeleteStation(CWProtocolMessage *msgPtr, int len, CWProtocolRequestStationValues *del_station);
CWBool CWParseMsgElemAddStation(CWProtocolMessage *msgPtr, int len, unsigned char *str);	
CWBool CWParseMsgElemFreeStation(CWProtocolMessage *msgPtr, int len, CWProtocolRequestStationValues *free_sta);
CWBool CWParseMsgElemSilentStation(CWProtocolMessage *msgPtr, int len, CWProtocolRequestStationValues *silent_sta);
CWBool CWParseMsgElemUnsilentStation(CWProtocolMessage *msgPtr, int len, CWProtocolRequestStationValues *unsilent_sta);
CWBool CWParseMsgElemDot11vStation(CWProtocolMessage *msgPtr, int len, CWProtocolRequestStationValues *dot11v_sta);
CWBool CWParseMsgElemDualStation(CWProtocolMessage *msgPtr, int len, CWProtocolRequestStationValues *dual_sta);
#endif
CWBool CWParseMsgElemImageIdentifier(CWProtocolMessage *msgPtr, int len, CWProtocolImageIdentifierValues *image_identifier);
#ifdef RTK_CAPWAP
CWBool CWParseMsgElemRtkChannel(CWProtocolMessage *msgPtr, int len, CWProtocolRtkChannelValues *valPtr);
CWBool CWParseMsgElemRtkPowerScale(CWProtocolMessage *msgPtr, int len, CWProtocolRtkPowerScaleValues *valPtr);
#ifndef KM_BY_AC
CWBool CWParseMsgElemRtkWlanConfiguration(CWProtocolMessage *msgPtr, int len, CWProtocolRtkWlanConfigValues *valPtr);	//65000
#endif
#endif


#endif
