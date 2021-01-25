#include <lcd_config/pre_hook.h>

#include "common.h"

#include <lcd_config/post_hook.h>

 netif_carrier_on(struct net_device* dev) {}
 netif_carrier_off(struct net_device* dev) {}
unsigned int register_netdevice(struct net_device* dev) {}
 consume_skb(struct sk_buff* skb) {}
struct net_device* alloc_netdev_mqs(unsigned int sizeof_priv, char* name, unsigned char name_assign_type, fptr_setup setup, unsigned int txqs, unsigned int rxqs) {}
 free_netdev(struct net_device* dev) {}
 rtnl_lock() {}
 rtnl_unlock() {}
 rtnl_link_unregister(struct rtnl_link_ops* _global_rtnl_link_ops) {}
 __rtnl_link_unregister(struct rtnl_link_ops* _global_rtnl_link_ops) {}
unsigned int __rtnl_link_register(struct rtnl_link_ops* _global_rtnl_link_ops) {}
 ether_setup(struct net_device* dev) {}
unsigned int eth_validate_addr(struct net_device* dev) {}
unsigned int eth_mac_addr(struct net_device* dev, struct sockaddr* p) {}
 trmp_setup(struct net_device* dev) {}
 trmp_impl_setup(fptr_setup target, struct net_device* dev) {}
 trmp_get_drvinfo(struct net_device* dev, struct ethtool_drvinfo* info) {}
 trmp_impl_get_drvinfo(fptr_get_drvinfo target, struct net_device* dev, struct ethtool_drvinfo* info) {}
unsigned int trmp_ndo_init(struct net_device* dev) {}
unsigned int trmp_impl_ndo_init(fptr_ndo_init target, struct net_device* dev) {}
 trmp_ndo_uninit(struct net_device* dev) {}
 trmp_impl_ndo_uninit(fptr_ndo_uninit target, struct net_device* dev) {}
unsigned long long trmp_ndo_start_xmit(struct sk_buff* skb, struct net_device* dev) {}
unsigned long long trmp_impl_ndo_start_xmit(fptr_ndo_start_xmit target, struct sk_buff* skb, struct net_device* dev) {}
 trmp_ndo_set_rx_mode(struct net_device* dev) {}
 trmp_impl_ndo_set_rx_mode(fptr_ndo_set_rx_mode target, struct net_device* dev) {}
struct rtnl_link_stats64* trmp_ndo_get_stats64(struct net_device* dev, struct rtnl_link_stats64* stats) {}
struct rtnl_link_stats64* trmp_impl_ndo_get_stats64(fptr_ndo_get_stats64 target, struct net_device* dev, struct rtnl_link_stats64* stats) {}
unsigned int trmp_ndo_change_carrier(struct net_device* dev, bool new_carrier) {}
unsigned int trmp_impl_ndo_change_carrier(fptr_ndo_change_carrier target, struct net_device* dev, bool new_carrier) {}
unsigned int trmp_validate(struct nlattr** tb, struct nlattr** data) {}
unsigned int trmp_impl_validate(fptr_validate target, struct nlattr** tb, struct nlattr** data) {}
