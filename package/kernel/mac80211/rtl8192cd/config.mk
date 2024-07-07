#
# RTL WLan AP Driver All In One Configure
#

CONFIG_RTL8192CD :=m
RTK_BSP :=n
EXTRA_CFLAGS += -DCONFIG_RTL8192CD
RTL_WLAN_DATA_DIR =

ifeq ($(RTK_BSP),n)
	EXTRA_CFLAGS += -DNOT_RTK_BSP
	EXTRA_CFLAGS += -DCONFIG_WIRELESS_LAN_MODULE
	CONFIG_OPENWRT_SDK :=y
endif

ifeq ($(CONFIG_PCI_HCI),y)
	EXTRA_CFLAGS += -DCONFIG_PCI_HCI
endif

ifeq ($(CONFIG_USB_HCI),y)
	EXTRA_CFLAGS += -DCONFIG_USB_HCI
endif

ifeq ($(CONFIG_SDIO_HCI),y)
	EXTRA_CFLAGS += -DCONFIG_SDIO_HCI
	# 0: No AP power saving 1: RF off  2: beacon offload
	CONFIG_AP_PS := 0
endif

ifeq ($(CONFIG_USE_PCIE_SLOT_0),y)
	EXTRA_CFLAGS += -DCONFIG_USE_PCIE_SLOT_0
endif

ifeq ($(CONFIG_USE_PCIE_SLOT_1),y)
	EXTRA_CFLAGS += -DCONFIG_USE_PCIE_SLOT_1
endif

ifeq ($(CONFIG_SLOT_0_TX_BEAMFORMING),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_TX_BEAMFORMING
	CONFIG_BEAMFORMING_SUPPORT:=y
endif

ifeq ($(CONFIG_SLOT_1_TX_BEAMFORMING),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_TX_BEAMFORMING
	CONFIG_BEAMFORMING_SUPPORT:=y
endif

ifeq ($(CONFIG_SLOT_0_8192EE),y)
	CONFIG_RTL_92E_SUPPORT :=y
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_8192EE
endif

ifeq ($(CONFIG_SLOT_1_8192EE),y)
	CONFIG_RTL_92E_SUPPORT :=y
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_8192EE
endif

ifeq ($(CONFIG_SLOT_0_8812),y)
	CONFIG_RTL_8812_SUPPORT :=y
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_8812
endif

ifeq ($(CONFIG_SLOT_1_8812),y)
	CONFIG_RTL_8812_SUPPORT :=y
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_8812
endif

ifeq ($(CONFIG_SLOT_0_8812AR_VN),y)
	CONFIG_RTL_8812_SUPPORT :=y
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_8812AR_VN -DCONFIG_RTL_8812AR_VN_SUPPORT
endif

ifeq ($(CONFIG_SLOT_1_8812AR_VN),y)
	CONFIG_RTL_8812_SUPPORT :=y
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_8812AR_VN -DCONFIG_RTL_8812AR_VN_SUPPORT
endif

ifeq ($(CONFIG_SLOT_0_8814AE),y)
	CONFIG_RTL_8814_SUPPORT :=y
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_8814AE
endif

ifeq ($(CONFIG_SLOT_1_8814AE),y)
	CONFIG_RTL_8814_SUPPORT :=y
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_8814AE
endif

ifeq ($(CONFIG_SLOT_0_8822BE),y)
	CONFIG_RTL_8822_SUPPORT :=y
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_8822BE
endif

ifeq ($(CONFIG_SLOT_1_8822BE),y)
	CONFIG_RTL_8822_SUPPORT :=y
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_88122BE
endif

ifeq ($(CONFIG_RTL_92C_SUPPORT),y)
	CONFIG_RTL_WLAN_HAL_NOT_EXIST :=y
	EXTRA_CFLAGS += -DCONFIG_RTL_92C_SUPPORT
	RTL_WLAN_DATA_DIR += data
endif

ifeq ($(CONFIG_RTL_92D_SUPPORT),y)
	CONFIG_RTL_WLAN_HAL_NOT_EXIST :=y
	EXTRA_CFLAGS += -DCONFIG_RTL_92D_SUPPORT
	#EXTRA_CFLAGS += -DCONFIG_RTL_92D_DMDP
	RTL_WLAN_DATA_DIR += data_92d
endif

ifeq ($(CONFIG_RTL_88E_SUPPORT),y)
	CONFIG_RTL_WLAN_HAL_NOT_EXIST :=y
	CONFIG_RTL_ODM_WLAN_DRIVER :=y
	EXTRA_CFLAGS += -DCONFIG_RTL_88E_SUPPORT
	RTL_WLAN_DATA_DIR += data_88e
endif

ifeq ($(CONFIG_RTL_8812_SUPPORT),y)
#	CONFIG_RTL_DFS_SUPPORT :=n
	CONFIG_RTL_WLAN_HAL_NOT_EXIST :=y
	CONFIG_RTL_ODM_WLAN_DRIVER :=y
	EXTRA_CFLAGS += -DCONFIG_RTL_8812_SUPPORT
	ifeq ($(CONFIG_BEAMFORMING_SUPPORT), y)
		EXTRA_CFLAGS += -DBEAMFORMING_SUPPORT
	endif
	RTL_WLAN_DATA_DIR += data_8812
endif

ifeq ($(CONFIG_RTL_8814_SUPPORT),y)
	CONFIG_WLAN_HAL :=y
	CONFIG_WLAN_HAL_88XX :=y
	CONFIG_WLAN_HAL_8814AE :=y
	CONFIG_RTL_ODM_WLAN_DRIVER :=y
	EXTRA_CFLAGS += -DCONFIG_WLAN_HAL_8814AE
	EXTRA_CFLAGS += -I$(RTL8192CD_DIR)/WlanHAL/RTL88XX/RTL8814A/RTL8814AE -I$(RTL8192CD_DIR)/WlanHAL/RTL88XX/RTL8814A	
	ifeq ($(CONFIG_BEAMFORMING_SUPPORT), y)
		EXTRA_CFLAGS += -DBEAMFORMING_SUPPORT
	endif
	RTL_WLAN_DATA_DIR += WlanHAL/Data/8814A
endif

ifeq ($(CONFIG_RTL_92E_SUPPORT),y)
	CONFIG_WLAN_HAL :=y
	CONFIG_WLAN_HAL_88XX :=y
	CONFIG_WLAN_HAL_8192EE :=y
	CONFIG_RTL_ODM_WLAN_DRIVER :=y
	EXTRA_CFLAGS += -DCONFIG_WLAN_HAL_8192EE
	EXTRA_CFLAGS += -I$(RTL8192CD_DIR)/WlanHAL/RTL88XX/RTL8192E/RTL8192EE -I$(RTL8192CD_DIR)/WlanHAL/RTL88XX/RTL8192E
	ifeq ($(CONFIG_BEAMFORMING_SUPPORT), y)
		EXTRA_CFLAGS += -DBEAMFORMING_SUPPORT
	endif
	RTL_WLAN_DATA_DIR += WlanHAL/Data/8192E
endif

ifeq ($(CONFIG_RTL_8822_SUPPORT),y)
	CONFIG_WLAN_HAL :=y
	CONFIG_WLAN_HAL_88XX :=y
	CONFIG_WLAN_HAL_8822BE :=y
	CONFIG_RTL_ODM_WLAN_DRIVER :=y
	CONFIG_WLAN_MACHAL_API :=y
	EXTRA_CFLAGS += -DCONFIG_WLAN_HAL_8822BE -DCONFIG_WLAN_MACHAL_API
	EXTRA_CFLAGS += -I$(RTL8192CD_DIR)/WlanHAL/RTL88XX/RTL8822B/RTL8822BE -I$(RTL8192CD_DIR)/WlanHAL/RTL88XX/RTL8822B
	RTL_WLAN_DATA_DIR += WlanHAL/Data/8822B
endif

ifeq ($(CONFIG_WLAN_HAL),y)	
	EXTRA_CFLAGS += -DCONFIG_WLAN_HAL
	EXTRA_CFLAGS += -I$(RTL8192CD_DIR) -I$(RTL8192CD_DIR)/WlanHAL/ -I$(RTL8192CD_DIR)/WlanHAL/Include -I$(RTL8192CD_DIR)/WlanHAL/HalHeader

	ifeq ($(CONFIG_WLAN_HAL_88XX),y)
		EXTRA_CFLAGS += -DCONFIG_WLAN_HAL_88XX
		EXTRA_CFLAGS += -I$(RTL8192CD_DIR)/WlanHAL/RTL88XX -I$(RTL8192CD_DIR)/WlanHAL/HalMac88XX
	endif

	ifeq ($(CONFIG_WLAN_HAL_8881A),y)
		RTL_WLAN_DATA_DIR := WlanHAL/Data/8881A
		EXTRA_CFLAGS += -I$(RTL8192CD_DIR)/WlanHAL/RTL88XX/RTL8881A
	endif
else
	EXTRA_CFLAGS += -DCONFIG_RTL_WLAN_HAL_NOT_EXIST
endif

ifeq ($(CONFIG_RTL_WLAN_HAL_NOT_EXIST),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_WLAN_HAL_NOT_EXIST
endif

ifeq ($(CONFIG_RTL_ODM_WLAN_DRIVER),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_ODM_WLAN_DRIVER
	EXTRA_CFLAGS += -I$(RTL8192CD_DIR) -I$(RTL8192CD_DIR)/phydm
endif

# TODO: We need to assign which NIC is using external PA and LNA
ifeq ($(CONFIG_SLOT_0_EXT_PA),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_EXT_PA
endif

ifeq ($(CONFIG_SLOT_1_EXT_PA),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_EXT_PA
endif

ifeq ($(CONFIG_SLOT_0_EXT_LNA),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_EXT_LNA
endif

ifeq ($(CONFIG_SLOT_1_EXT_LNA),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_EXT_LNA
endif

ifeq ($(CONFIG_SLOT_0_RFE_TYPE_0),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_RFE_TYPE_0
endif

ifeq ($(CONFIG_SLOT_1_RFE_TYPE_0),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_RFE_TYPE_0
endif

ifeq ($(CONFIG_SLOT_0_RFE_TYPE_1),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_RFE_TYPE_1
endif

ifeq ($(CONFIG_SLOT_1_RFE_TYPE_1),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_RFE_TYPE_1
endif

ifeq ($(CONFIG_SLOT_0_RFE_TYPE_2),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_RFE_TYPE_2
endif

ifeq ($(CONFIG_SLOT_1_RFE_TYPE_2),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_RFE_TYPE_2
endif

ifeq ($(CONFIG_SLOT_0_RFE_TYPE_3),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_RFE_TYPE_3
endif

ifeq ($(CONFIG_SLOT_1_RFE_TYPE_3),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_RFE_TYPE_3
endif

ifeq ($(CONFIG_SLOT_0_RFE_TYPE_4),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_RFE_TYPE_4
endif

ifeq ($(CONFIG_SLOT_1_RFE_TYPE_4),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_RFE_TYPE_4
endif

ifeq ($(CONFIG_SLOT_0_RFE_TYPE_5),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_RFE_TYPE_5
endif

ifeq ($(CONFIG_SLOT_1_RFE_TYPE_5),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_RFE_TYPE_5
endif

ifeq ($(CONFIG_SLOT_0_RFE_TYPE_6),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_RFE_TYPE_6
endif

ifeq ($(CONFIG_SLOT_1_RFE_TYPE_6),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_RFE_TYPE_6
endif

ifeq ($(CONFIG_SLOT_0_RFE_TYPE_7),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_RFE_TYPE_7
endif

ifeq ($(CONFIG_SLOT_1_RFE_TYPE_7),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_RFE_TYPE_7
endif

ifeq ($(CONFIG_SLOT_0_ANT_SWITCH),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_ANT_SWITCH
endif

ifeq ($(CONFIG_SLOT_1_ANT_SWITCH),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_ANT_SWITCH
endif

ifeq ($(CONFIG_NO_2G_DIVERSITY),y)
	EXTRA_CFLAGS += -DCONFIG_NO_2G_DIVERSITY
endif

ifeq ($(CONFIG_2G_CGCS_RX_DIVERSITY),y)
	EXTRA_CFLAGS += -DCONFIG_2G_CGCS_RX_DIVERSITY
endif

ifeq ($(CONFIG_2G_CG_TRX_DIVERSITY),y)
	EXTRA_CFLAGS += -DCONFIG_2G_CG_TRX_DIVERSITY
endif

ifeq ($(CONFIG_NO_5G_DIVERSITY),y)
	EXTRA_CFLAGS += -DCONFIG_NO_5G_DIVERSITY
endif

ifeq ($(CONFIG_5G_CGCS_RX_DIVERSITY),y)
	EXTRA_CFLAGS += -DCONFIG_5G_CGCS_RX_DIVERSITY
endif

ifeq ($(CONFIG_5G_CG_TRX_DIVERSITY),y)
	EXTRA_CFLAGS += -DCONFIG_5G_CG_TRX_DIVERSITY
endif

ifeq ($(CONFIG_RTL_DFS_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_DFS_SUPPORT
endif

ifeq ($(CONFIG_RTL_VAP_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_VAP_SUPPORT
endif

ifeq ($(CONFIG_RTL_CLIENT_MODE_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_CLIENT_MODE_SUPPORT
endif

ifeq ($(CONFIG_RTL_REPEATER_MODE_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_REPEATER_MODE_SUPPORT
endif

ifeq ($(CONFIG_RTL_SUPPORT_MULTI_PROFILE),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_SUPPORT_MULTI_PROFILE
endif

ifeq ($(CONFIG_RTL_MULTI_CLONE_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_MULTI_CLONE_SUPPORT
endif

ifeq ($(CONFIG_RTL_WDS_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_WDS_SUPPORT
endif

ifeq ($(CONFIG_SLOT_0_ENABLE_EFUSE),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_0_ENABLE_EFUSE -DCONFIG_ENABLE_EFUSE
endif

ifeq ($(CONFIG_SLOT_1_ENABLE_EFUSE),y)
	EXTRA_CFLAGS += -DCONFIG_SLOT_1_ENABLE_EFUSE -DCONFIG_ENABLE_EFUSE
endif

ifeq ($(CONFIG_RTL_COMAPI_CFGFILE),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_COMAPI_CFGFILE
endif

ifeq ($(CONFIG_RTL_COMAPI_WLTOOLS),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_COMAPI_WLTOOLS
endif

ifeq ($(CONFIG_WPA_CLI),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_COMAPI_WLTOOLS -DWIFI_WPAS_CLI -DSDIO_2_PORT
endif

ifeq ($(CONFIG_MP_PSD_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_MP_PSD_SUPPORT
endif

ifeq ($(CONFIG_RTL_P2P_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_P2P_SUPPORT
endif

ifeq ($(CONFIG_TXPWR_LMT),y)
	EXTRA_CFLAGS += -DCONFIG_TXPWR_LMT
endif

ifeq ($(CONFIG_RTL_MESH_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_MESH_SUPPORT
	EXTRA_CFLAGS += -DCONFIG_RTK_MESH
endif

ifeq ($(CONFIG_RTL_WLAN_DOS_FILTER),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_WLAN_DOS_FILTER
endif

ifeq ($(CONCURRENT_MODE),y)
	EXTRA_CFLAGS += -DCONCURRENT_MODE
endif

ifeq ($(CONFIG_RTL8190_PRIV_SKB),y)
	EXTRA_CFLAGS += -DCONFIG_RTL8190_PRIV_SKB
endif

ifeq ($(CONFIG_RTL_WPS2_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_WPS2_SUPPORT
endif

ifeq ($(CONFIG_PHY_EAT_40MHZ),y)
	EXTRA_CFLAGS += -DCONFIG_PHY_EAT_40MHZ
endif
ifeq ($(CONFIG_PHY_WLAN_EAT_40MHZ),y)
	EXTRA_CFLAGS += -DCONFIG_PHY_WLAN_EAT_40MHZ
endif

ifeq ($(CONFIG_RTL_TPT_THREAD),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_TPT_THREAD
endif

ifeq ($(CONFIG_RTL_A4_STA_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_A4_STA_SUPPORT
endif

ifeq ($(CONFIG_RTL_HS2_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_HS2_SUPPORT
endif

ifeq ($(CONFIG_PACP_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_PACP_SUPPORT
endif

ifeq ($(CONFIG_RTL_TDLS_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_TDLS_SUPPORT
endif

ifeq ($(CONFIG_RTL_STA_CONTROL_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_STA_CONTROL_SUPPORT
endif

ifeq ($(CONFIG_RTK_SMART_ROAMING),y)
	EXTRA_CFLAGS += -DCONFIG_RTK_SMART_ROAMING
endif

ifeq ($(CONFIG_RTL_SIMPLE_CONFIG),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_SIMPLE_CONFIG
endif

ifeq ($(CONFIG_RTL_WAPI_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_WAPI_SUPPORT
endif

ifeq ($(CONFIG_IGMP_SNOOPING_SUPPORT),y)
	EXTRA_CFLAGS += -D_FULLY_WIFI_IGMP_SNOOPING_SUPPORT_
endif

ifeq ($(CONFIG_MLD_SNOOPING_SUPPORT),y)
	EXTRA_CFLAGS += -D_FULLY_WIFI_MLD_SNOOPING_SUPPORT_
endif

ifeq ($(CONFIG_POWER_SAVE),y)
	EXTRA_CFLAGS += -DSDIO_AP_OFFLOAD -DCONFIG_POWER_SAVE
endif

ifeq ($(CONFIG_RTL_11W_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_11W_SUPPORT
endif

ifeq ($(CONFIG_RTL_11R_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_11R_SUPPORT
endif

ifeq ($(CONFIG_RTL_DOT11K_SUPPORT),y)
        EXTRA_CFLAGS += -DCONFIG_RTL_DOT11K_SUPPORT
endif

ifeq ($(CONFIG_RTL_11V_SUPPORT),y)
        EXTRA_CFLAGS += -DCONFIG_RTL_11V_SUPPORT
endif

ifeq ($(CONFIG_BAND_2G_ON_WLAN0),y)
	EXTRA_CFLAGS += -DCONFIG_BAND_2G_ON_WLAN0
endif

ifeq ($(CONFIG_BAND_5G_ON_WLAN0),y)
	EXTRA_CFLAGS += -DCONFIG_BAND_5G_ON_WLAN0
endif

ifeq ($(CONFIG_RTL_5G_SLOT_0),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_5G_SLOT_0
endif

ifeq ($(CONFIG_RTL_5G_SLOT_1),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_5G_SLOT_1
endif

ifeq ($(CONFIG_LNA_TYPE_0),y)
	EXTRA_CFLAGS += -DCONFIG_LNA_TYPE_0
endif

ifeq ($(CONFIG_LNA_TYPE_1),y)
	EXTRA_CFLAGS += -DCONFIG_LNA_TYPE_1
endif

ifeq ($(CONFIG_LNA_TYPE_2),y)
	EXTRA_CFLAGS += -DCONFIG_LNA_TYPE_2
endif

ifeq ($(CONFIG_LNA_TYPE_3),y)
	EXTRA_CFLAGS += -DCONFIG_LNA_TYPE_3
endif

ifeq ($(CONFIG_LNA_FROM_EFUSE),y)
	EXTRA_CFLAGS += -DCONFIG_LNA_FROM_EFUSE
endif

ifeq ($(CONFIG_PA_SKYWORKS_5022),y)
	EXTRA_CFLAGS += -DCONFIG_PA_SKYWORKS_5022
endif

ifeq ($(CONFIG_PA_RFMD_4501),y)
	EXTRA_CFLAGS += -DCONFIG_PA_RFMD_4501
endif

ifeq ($(CONFIG_PA_SKYWORKS_5023),y)
	EXTRA_CFLAGS += -DCONFIG_PA_SKYWORKS_5023
endif

ifeq ($(CONFIG_PA_SKYWORKS_85712_HP),y)
	EXTRA_CFLAGS += -DCONFIG_PA_SKYWORKS_85712_HP
endif

ifeq ($(CONFIG_RTL_8814_8194_2T2R_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_8814_8194_2T2R_SUPPORT
endif

#ifeq ($(CONFIG_BT_COEXIST_92EE),y)
#	EXTRA_CFLAGS += -DCONFIG_BT_COEXIST_92EE
#endif

ifeq ($(CONFIG_BT_COEXIST_92EE_OLD),y)
	EXTRA_CFLAGS += -DCONFIG_BT_COEXIST_92EE
endif

ifeq ($(CONFIG_BT_COEXIST_92EE_NEW),y)
	EXTRA_CFLAGS += -DCONFIG_BT_COEXIST
endif

ifeq ($(CONFIG_RTL_ATM_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_ATM_SUPPORT
endif

ifeq ($(CONFIG_RTK_WLAN_EVENT_INDICATE),y)
	EXTRA_CFLAGS += -DCONFIG_RTK_WLAN_EVENT_INDICATE
endif

ifeq ($(CONFIG_RTL_PMKCACHE_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_RTL_PMKCACHE_SUPPORT
endif

ifeq ($(CONFIG_RTD129X_MESH_LED),y)
	EXTRA_CFLAGS += -DCONFIG_RTD129X_MESH_LED
endif

ifeq ($(CONFIG_RTD129X_IGPIO_LED),y)
	EXTRA_CFLAGS += -DCONFIG_RTD129X_IGPIO_LED
endif

ifeq ($(CONFIG_KERNEL_RTL_BR_SHORTCUT),y)
	EXTRA_CFLAGS += -DBR_SHORTCUT_SUPPORT
endif

ifeq ($(CONFIG_RTL_SMP_LOAD_BALANCE_SUPPORT),y)
	EXTRA_CFLAGS += -DCONFIG_SMP_LOAD_BALANCE_SUPPORT
endif
