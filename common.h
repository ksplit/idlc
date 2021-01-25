#ifndef COMMON_H
#define COMMON_H

enum RPC_ID {
	RPC_ID_netif_carrier_on,
	RPC_ID_netif_carrier_off,
	RPC_ID_register_netdevice,
	RPC_ID_consume_skb,
	RPC_ID_alloc_netdev_mqs,
	RPC_ID_free_netdev,
	RPC_ID_rtnl_lock,
	RPC_ID_rtnl_unlock,
	RPC_ID_rtnl_link_unregister,
	RPC_ID___rtnl_link_unregister,
	RPC_ID___rtnl_link_register,
	RPC_ID_ether_setup,
	RPC_ID_eth_validate_addr,
	RPC_ID_eth_mac_addr,
	RPC_ID_setup,
	RPC_ID_get_drvinfo,
	RPC_ID_ndo_init,
	RPC_ID_ndo_uninit,
	RPC_ID_ndo_start_xmit,
	RPC_ID_ndo_set_rx_mode,
	RPC_ID_ndo_get_stats64,
	RPC_ID_ndo_change_carrier,
	RPC_ID_validate,
};

typedef void (*fptr_setup)(void);
typedef void (*fptr_get_drvinfo)(void);
typedef void (*fptr_ndo_init)(void);
typedef void (*fptr_ndo_uninit)(void);
typedef void (*fptr_ndo_start_xmit)(void);
typedef void (*fptr_ndo_set_rx_mode)(void);
typedef void (*fptr_ndo_get_stats64)(void);
typedef void (*fptr_ndo_change_carrier)(void);
typedef void (*fptr_validate)(void);

#endif
