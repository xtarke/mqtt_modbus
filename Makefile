PROGRAM=mqtt_client
#EXTRA_COMPONENTS = extras/dhcpserver extras/wificfg extras/paho_mqtt_c
EXTRA_COMPONENTS = extras/dhcpserver ./wificfg extras/paho_mqtt_c
# include ../../common.mk
EXTRA_CFLAGS=-DLWIP_MDNS_RESPONDER=1 -DLWIP_NUM_NETIF_CLIENT_DATA=1 -DLWIP_NETIF_EXT_STATUS_CALLBACK=1
include /home/xtarke/Data/Apps/esp-open-rtos/common.mk
