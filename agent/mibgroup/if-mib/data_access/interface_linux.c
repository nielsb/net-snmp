/*
 *  Interface MIB architecture support
 *
 * $Id$
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-features.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/snmp_agent.h>
#include <net-snmp/agent/snmp_vars.h>
#include "interface_private.h"

netsnmp_feature_require(fd_event_manager);
netsnmp_feature_require(delete_prefix_info);
netsnmp_feature_require(create_prefix_info);
netsnmp_feature_child_of(interface_arch_set_admin_status, interface_all);

#ifdef NETSNMP_FEATURE_REQUIRE_INTERFACE_ARCH_SET_ADMIN_STATUS
netsnmp_feature_require(interface_ioctl_flags_set);
#endif /* NETSNMP_FEATURE_REQUIRE_INTERFACE_ARCH_SET_ADMIN_STATUS */

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifndef HAVE_LIBNL3
#error libnl-3 is required. Please install the libnl-3 and libnl-route-3 development packages and remove --without-nl from the configure options if necessary.
#endif
#include <netlink/cache.h>
#include <netlink/netlink.h>
#include <netlink/route/addr.h>

#ifdef HAVE_PCI_LOOKUP_NAME
#include <pci/pci.h>
#include <setjmp.h>
#ifndef PCI_NONRET
#define PCI_NONRET
#endif
static struct pci_access *pci_access;

/* Avoid letting libpci call exit(1) when no PCI bus is available. */
static int do_longjmp =0;
static jmp_buf err_buf;
PCI_NONRET static void
netsnmp_pci_error(char *msg, ...)
{
    va_list args;
    char *buf;
    int buflen;

    va_start(args, msg);
    buflen = strlen("pcilib: ")+strlen(msg)+2;
    buf = malloc(buflen);
    snprintf(buf, buflen, "pcilib: %s\n", msg);
    snmp_vlog(LOG_ERR, buf, args);
    free(buf);
    va_end(args);
    if (do_longjmp)
	longjmp(err_buf, 1);
    else
	exit(1);
}
#endif

#ifdef HAVE_LINUX_ETHTOOL_H
#ifdef HAVE_LINUX_ETHTOOL_NEEDS_U64
#include <linux/types.h>
typedef __u64 u64;         /* hack, so we may include kernel's ethtool.h */
typedef __u32 u32;         /* ditto */
typedef __u16 u16;         /* ditto */
typedef __u8 u8;           /* ditto */
#endif
#include <linux/ethtool.h>
#endif /* HAVE_LINUX_ETHTOOL_H */

#include "mibII/mibII_common.h"
#include "if-mib/ifTable/ifTable_constants.h"
#include "ip-mib/data_access/ipaddress_ioctl.h"

#include <net-snmp/agent/net-snmp-agent-includes.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#else
#error "linux should have sys/ioctl header"
#endif

#include <net-snmp/data_access/interface.h>
#include <net-snmp/data_access/ipaddress.h>
#include "if-mib/data_access/interface.h"
#include "mibgroup/util_funcs.h"
#include "interface_ioctl.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <linux/sockios.h>
#include <linux/if_ether.h>

#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#endif

#ifndef SIOCGMIIPHY
#define SIOCGMIIPHY 0x8947
#endif

#ifndef SIOCGMIIREG
#define SIOCGMIIREG 0x8948
#endif

#ifdef NETSNMP_ENABLE_IPV6
#if defined(HAVE_LINUX_RTNETLINK_H)
#include <linux/rtnetlink.h>
#ifdef RTMGRP_IPV6_PREFIX
#define SUPPORT_PREFIX_FLAGS 1
#endif  /* RTMGRP_IPV6_PREFIX */
#endif  /* HAVE_LINUX_RTNETLINK_H */
#endif  /* NETSNMP_ENABLE_IPV6 */
unsigned long long
netsnmp_linux_interface_get_if_speed(int fd, const char *name,
        unsigned long long defaultspeed);
#ifdef HAVE_LINUX_ETHTOOL_H
unsigned long long
netsnmp_linux_interface_get_if_speed_mii(int fd, const char *name,
        unsigned long long defaultspeed);
#endif

#define PROC_SYS_NET_IPVx_NEIGH_RETRANS_TIME_MS "/proc/sys/net/ipv%d/neigh/%s/retrans_time_ms"
#define PROC_SYS_NET_IPVx_NEIGH_RETRANS_TIME    "/proc/sys/net/ipv%d/neigh/%s/retrans_time"
#define PROC_SYS_NET_IPVx_BASE_REACHABLE_TIME_MS "/proc/sys/net/ipv%d/neigh/%s/base_reachable_time_ms"
#define PROC_SYS_NET_IPVx_BASE_REACHABLE_TIME "/proc/sys/net/ipv%d/neigh/%s/base_reachable_time"
#ifdef SUPPORT_PREFIX_FLAGS
prefix_cbx *prefix_head_list = NULL;
netsnmp_prefix_listen_info list_info;
#define IF_PREFIX_ONLINK        0x01
#define IF_PREFIX_AUTOCONF      0x02
 
int netsnmp_prefix_listen(void);
#endif

#ifdef HAVE_PCI_LOOKUP_NAME
static void init_libpci(void)
{
    struct stat stbuf;

    /*
     * When snmpd is run inside an OpenVZ container or on a Raspberry Pi system
     * /proc/bus/pci is not available.
     */
    if (stat("/proc/bus/pci", &stbuf) < 0)
        return;

    pci_access = pci_alloc();
    if (!pci_access) {
	snmp_log(LOG_ERR, "pcilib: pci_alloc failed\n");
	return;
    }

    pci_access->error = netsnmp_pci_error;

    do_longjmp = 1;
    if (setjmp(err_buf)) {
        pci_cleanup(pci_access);
	snmp_log(LOG_ERR, "pcilib: pci_init failed\n");
        pci_access = NULL;
    }
    else if (pci_access)
	pci_init(pci_access);
    do_longjmp = 0;
}
#else
static void init_libpci(void)
{
}
#endif

void
netsnmp_arch_interface_init(void)
{
#ifdef SUPPORT_PREFIX_FLAGS
    list_info.list_head = &prefix_head_list;
    netsnmp_prefix_listen();
#endif

    init_libpci();
}

/*
 * find the ifIndex for an interface name
 * NOTE: The Linux version is not efficient for large numbers of calls.
 *   consider using netsnmp_access_interface_ioctl_ifindex_get()
 *   for loops which need to look up a lot of indexes.
 *
 * @retval 0 : no index found
 * @retval >0: ifIndex for interface
 */
oid
netsnmp_arch_interface_index_find(const char *name)
{
    return netsnmp_access_interface_ioctl_ifindex_get(-1, name);
}


/*
 * check for ipv6 addresses
 */
void
_arch_interface_has_ipv6(oid if_index, u_int *flags,
                         netsnmp_container *addr_container)
{
#ifdef NETSNMP_ENABLE_IPV6
    netsnmp_ipaddress_entry *addr_entry = NULL;
    netsnmp_iterator        *addr_it = NULL;
    u_int                    addr_container_flags = 0; /* must init to 0 */
#endif

    if (NULL == flags)
        return;

    *flags &= ~NETSNMP_INTERFACE_FLAGS_HAS_IPV6;

#ifdef NETSNMP_ENABLE_IPV6
    /*
     * get ipv6 addresses
     */
    if (NULL == addr_container) {
        /*
         * we only care about ipv6, if we need to allocate our own
         * temporary container. set the flags (which we also use later
         * to determine if we need to free the container).
         */
        addr_container_flags = NETSNMP_ACCESS_IPADDRESS_LOAD_IPV6_ONLY;
        addr_container =
            netsnmp_access_ipaddress_container_load(NULL,
                                                    addr_container_flags);
        if (NULL == addr_container) {
            DEBUGMSGTL(("access:ifcontainer",
                        "couldn't get ip addresses container\n"));
            return;
        }
    }
    else {
        /*
         * addr_container flags must be 0, so we don't release the
         * user's container.
         */
        netsnmp_assert(0 == addr_container_flags);
    }


    /*
     * get an ipaddress container iterator, and look for ipv6 addrs
     */   
    addr_it = CONTAINER_ITERATOR(addr_container);
    if (NULL == addr_it) {
        DEBUGMSGTL(("access:ifcontainer",
                    "couldn't get ip addresses iterator\n"));
        if (0!=addr_container_flags)
            netsnmp_access_ipaddress_container_free(addr_container, 0);
        return;
    }

    addr_entry = ITERATOR_FIRST(addr_it);
    for( ; addr_entry ; addr_entry = ITERATOR_NEXT(addr_it) ) {
        /*
         * skip non matching indexes and ipv4 addresses
         */
        if ((if_index != addr_entry->if_index) ||
            (4 == addr_entry->ia_address_len))
            continue;

        /*
         * found one! no need to keep looking, set the flag and bail
         */
        *flags |= NETSNMP_INTERFACE_FLAGS_HAS_IPV6;
        break;
    }

    /*
     * make mama proud and clean up after ourselves
     */
    ITERATOR_RELEASE(addr_it);
    if (0!=addr_container_flags)
        netsnmp_access_ipaddress_container_free(addr_container, 0);    
#endif
}

/**
 * @internal
 */
static void
_arch_interface_flags_v4_get(netsnmp_interface_entry *entry)
{
    FILE           *fin;
    char            line[256];
    struct          stat st;
    unsigned short retrans_time_factor;

    /*
     * Check which retransmit time interface is available
     */
    snprintf(line, sizeof(line), PROC_SYS_NET_IPVx_NEIGH_RETRANS_TIME_MS, 4,
             entry->name);
    if (stat(line, &st) == 0) {
        retrans_time_factor = 1;
    } else {
        snprintf(line, sizeof(line), PROC_SYS_NET_IPVx_NEIGH_RETRANS_TIME, 4,
                 entry->name);
        retrans_time_factor = 10;
    }

    /*
     * get the retransmit time
     */
    if (!(fin = fopen(line, "r"))) {
        DEBUGMSGTL(("access:interface",
                    "Failed to open %s\n", line));
    } else {
        if (fgets(line, sizeof(line), fin)) {
            entry->retransmit_v4 = atoi(line) * retrans_time_factor;
            entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_V4_RETRANSMIT;
        }
        fclose(fin);
    }
}

#ifdef HAVE_PCI_LOOKUP_NAME

/* Get value from sysfs file */
static int sysfs_get_id(const char *path, unsigned short *id)
{
    FILE *fin;
    int n;

    if (!(fin = fopen(path, "r"))) {
        DEBUGMSGTL(("access:interface",
                    "Failed to open %s\n", path));
	return 0;
    }

    n = fscanf(fin, "%hx", id);
    fclose(fin);

    return n == 1;
}

/* Get interface description for PCI device
 * by using sysfs to find vendor and device
 * then lookup name (-lpci)
 *
 * For software interfaces there is no PCI information
 * so description will not be set.
 */
static void
_arch_interface_description_get(netsnmp_interface_entry *entry)
{
    const char *descr;
    char buf[256];
    unsigned short vendor_id, device_id;

    if (!pci_access)
	return;

    snprintf(buf, sizeof(buf),
	     "/sys/class/net/%s/device/vendor", entry->name);

    if (!sysfs_get_id(buf, &vendor_id))
	return;

    snprintf(buf, sizeof(buf),
	     "/sys/class/net/%s/device/device", entry->name);

    if (!sysfs_get_id(buf, &device_id))
	return;

    descr = pci_lookup_name(pci_access, buf, sizeof(buf),
			    PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE,
			    vendor_id, device_id, 0, 0);
    if (descr) {
	free(entry->descr);
	entry->descr = strdup(descr);
    } else {
        DEBUGMSGTL(("access:interface",
                    "Failed pci_lookup_name vendor=%#hx device=%#hx\n",
		    vendor_id, device_id));
    }
}
#endif


#ifdef NETSNMP_ENABLE_IPV6
/**
 * @internal
 */
static void
_arch_interface_flags_v6_get(netsnmp_interface_entry *entry)
{
    FILE           *fin;
    char            line[256];
    struct          stat st;
    unsigned short retrans_time_factor;
    unsigned short basereachable_time_ms;

    /*
     * Check which retransmit time interface is available
     */
    snprintf(line, sizeof(line), PROC_SYS_NET_IPVx_NEIGH_RETRANS_TIME_MS, 6,
             entry->name);
    if (stat(line, &st) == 0) {
        retrans_time_factor = 1;
    } else {
        snprintf(line, sizeof(line), PROC_SYS_NET_IPVx_NEIGH_RETRANS_TIME, 6,
                 entry->name);
        retrans_time_factor = 10;
    }
	
    /*
     * get the retransmit time
     */
    if (!(fin = fopen(line, "r"))) {
        DEBUGMSGTL(("access:interface",
                    "Failed to open %s\n", line));
    }
    else {
        if (fgets(line, sizeof(line), fin)) {
            entry->retransmit_v6 = atoi(line) * retrans_time_factor;
            entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_V6_RETRANSMIT;
        }
        fclose(fin);
    }

    /*
     * get the forwarding status
     */
    snprintf(line, sizeof(line), "/proc/sys/net/ipv6/conf/%s/forwarding",
             entry->name);
    if (!(fin = fopen(line, "r"))) {
        DEBUGMSGTL(("access:interface",
                    "Failed to open %s\n", line));
    }
    else {
        if (fgets(line, sizeof(line), fin)) {
            entry->forwarding_v6 = atoi(line);
            entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_V6_FORWARDING;
        }
        fclose(fin);
    }
	
    /*
     *  Check which base reachable time interface is available.
     */

    snprintf(line, sizeof(line), PROC_SYS_NET_IPVx_BASE_REACHABLE_TIME_MS, 6,
             entry->name);
    if (stat(line, &st) == 0) {
        basereachable_time_ms = 1;
    } else {
        snprintf(line, sizeof(line), PROC_SYS_NET_IPVx_BASE_REACHABLE_TIME, 6,
                 entry->name);
        basereachable_time_ms = 0;
    }

    /*
     * get the reachable time
     */
    if (!(fin = fopen(line, "r"))) {
        DEBUGMSGTL(("access:interface",
                    "Failed to open %s\n", line));
    }
    else {
        if (fgets(line, sizeof(line), fin)) {
            if (basereachable_time_ms) {
                entry->reachable_time = atoi(line); /* millisec */
            } else {
                entry->reachable_time = atoi(line)*1000; /* sec to  millisec */
            }

            entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_V6_REACHABLE;
        }
        fclose(fin);
    }
}
#endif /* NETSNMP_ENABLE_IPV6 */

/* Guess the IANA network interface type from the network interface name. */
static int netsnmp_guess_interface_type(const netsnmp_interface_entry *entry)
{
    struct match_if {
        int             mi_type;
        const char     *mi_name;
    };

    static const struct match_if lmatch_if[] = {
        {IANAIFTYPE_SOFTWARELOOPBACK, "lo"},
        {IANAIFTYPE_ETHERNETCSMACD, "eth"},
        {IANAIFTYPE_ETHERNETCSMACD, "vmnet"},
        {IANAIFTYPE_ISO88025TOKENRING, "tr"},
        {IANAIFTYPE_FASTETHER, "feth"},
        {IANAIFTYPE_GIGABITETHERNET,"gig"},
        {IANAIFTYPE_INFINIBAND,"ib"},
        {IANAIFTYPE_PPP, "ppp"},
        {IANAIFTYPE_SLIP, "sl"},
        {IANAIFTYPE_TUNNEL, "sit"},
        {IANAIFTYPE_BASICISDN, "ippp"},
        {IANAIFTYPE_PROPVIRTUAL, "bond"}, /* Bonding driver find fastest slave */
        {IANAIFTYPE_PROPVIRTUAL, "vad"},  /* ANS driver - ?speed? */
        {0, NULL}                  /* end of list */
    };

    const struct match_if *pm;

    for (pm = lmatch_if; pm->mi_name; pm++) {
        const int len = strlen(pm->mi_name);

        if (strncmp(entry->name, pm->mi_name, len) == 0)
            return pm->mi_type;
    }
    return IANAIFTYPE_OTHER;
}

static void netsnmp_derive_interface_id(netsnmp_interface_entry *entry)
{
    /*
     * interface identifier is specified based on physaddr and type
     */
    switch (entry->type) {
    case IANAIFTYPE_ETHERNETCSMACD:
    case IANAIFTYPE_ETHERNET3MBIT:
    case IANAIFTYPE_FASTETHER:
    case IANAIFTYPE_FASTETHERFX:
    case IANAIFTYPE_GIGABITETHERNET:
    case IANAIFTYPE_FDDI:
    case IANAIFTYPE_ISO88025TOKENRING:
        if (!entry->paddr || entry->paddr_len != ETH_ALEN)
            break;

        entry->v6_if_id_len = entry->paddr_len + 2;
        memcpy(entry->v6_if_id, entry->paddr, 3);
        memcpy(entry->v6_if_id + 5, entry->paddr + 3, 3);
        entry->v6_if_id[0] ^= 2;
        entry->v6_if_id[3] = 0xFF;
        entry->v6_if_id[4] = 0xFE;

        entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_V6_IFID;
        break;

    case IANAIFTYPE_SOFTWARELOOPBACK:
        entry->v6_if_id_len = 0;
        entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_V6_IFID;
        break;
    }
}

static void netsnmp_retrieve_link_speed(int fd, netsnmp_interface_entry *entry)
{
    unsigned long long defaultspeed = NOMINAL_LINK_SPEED, speed;

    if (!(entry->os_flags & IFF_RUNNING)) {
        /*
         * use speed 0 if the if speed cannot be determined *and* the
         * interface is down
         */
        defaultspeed = 0;
    }
    speed = netsnmp_linux_interface_get_if_speed(fd, entry->name,
                                                 defaultspeed);
    entry->speed_high = speed / 1000000LL;
    entry->speed = speed;
    if (speed > 0xffffffff)
        entry->speed = 0xffffffff; /* 4294967295; */
}

/**
 * @internal
 */
static void
_retrieve_stats(netsnmp_interface_entry *entry, struct rtnl_link *rtnl_link)
{
    uint64_t rec_oct, rec_pkt, rec_err, rec_drop, rec_mcast, snd_oct;
    uint64_t snd_pkt, snd_err, snd_drop, coll;

    /*
     * See also dev_seq_printf_stats() in the Linux kernel source file
     * net/core/net-procfs.c.
     */
    rec_oct = rtnl_link_get_stat(rtnl_link, RTNL_LINK_RX_BYTES);
    rec_pkt = rtnl_link_get_stat(rtnl_link, RTNL_LINK_RX_PACKETS);
    rec_err = rtnl_link_get_stat(rtnl_link, RTNL_LINK_RX_ERRORS);
    rec_drop = rtnl_link_get_stat(rtnl_link, RTNL_LINK_RX_DROPPED) +
        rtnl_link_get_stat(rtnl_link, RTNL_LINK_RX_MISSED_ERR);
    rec_mcast = rtnl_link_get_stat(rtnl_link, RTNL_LINK_MULTICAST);
    /*
     * Some libnl versions incorrectly report zero received packets. If that
     * is the case, fetch the number of received packets from sysfs.
     */
    if (rec_oct && rec_pkt == 0) {
        char path[256];
        FILE *f;

        snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/rx_packets",
                 entry->name);
        f = fopen(path, "rb");
        if (f) {
            NETSNMP_IGNORE_RESULT(fscanf(f, "%" SCNu64, &rec_pkt));
            fclose(f);
        }
        if (rec_pkt < rec_mcast)
            rec_pkt = rec_mcast;
        DEBUGMSGTL(("access:ifcontainer",
                    "%s: rec_oct = %" PRIu64 ", rec_pkt = 0 -> %" PRIu64 "\n",
                    entry->name, rec_oct, rec_pkt));
    }
    snd_oct = rtnl_link_get_stat(rtnl_link, RTNL_LINK_TX_BYTES);
    snd_pkt = rtnl_link_get_stat(rtnl_link, RTNL_LINK_TX_PACKETS);
    snd_err = rtnl_link_get_stat(rtnl_link, RTNL_LINK_TX_ERRORS);
    snd_drop = rtnl_link_get_stat(rtnl_link, RTNL_LINK_TX_DROPPED);
    coll = rtnl_link_get_stat(rtnl_link, RTNL_LINK_COLLISIONS);

    entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_BYTES;
    entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_DROPS;
    entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_MCAST_PKTS;
    entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_HIGH_SPEED;
    entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_HIGH_BYTES;
    entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_HIGH_PACKETS;
    entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_ACTIVE;
    /*
     * subtract out multicast packets from rec_pkt before
     * we store it as unicast counter.
     */
    entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_CALCULATE_UCAST;
    entry->stats.ibytes.low = rec_oct & 0xffffffff;
    entry->stats.iall.low = rec_pkt & 0xffffffff;
    entry->stats.imcast.low = rec_mcast & 0xffffffff;
    entry->stats.obytes.low = snd_oct & 0xffffffff;
    entry->stats.oucast.low = snd_pkt & 0xffffffff;
    entry->stats.ibytes.high = rec_oct >> 32;
    entry->stats.iall.high = rec_pkt >> 32;
    entry->stats.imcast.high = rec_mcast >> 32;
    entry->stats.obytes.high = snd_oct >> 32;
    entry->stats.oucast.high = snd_pkt >> 32;
    entry->stats.ierrors   = rec_err;
    entry->stats.idiscards = rec_drop;
    entry->stats.oerrors   = snd_err;
    entry->stats.odiscards = snd_drop;
    entry->stats.collisions = coll;

    /*
     * calculated stats.
     *
     *  we have imcast, but not ibcast.
     */
    entry->stats.inucast = entry->stats.imcast.low +
        entry->stats.ibcast.low;
    entry->stats.onucast = entry->stats.omcast.low +
        entry->stats.obcast.low;
}

static void netsnmp_retrieve_one_link_info(struct rtnl_link *rtnl_link, int fd,
                                           netsnmp_interface_entry *entry,
                                           int load_stats)
{
    struct nl_addr *nl_addr = rtnl_link_get_addr(rtnl_link);
    void *paddr = nl_addr ? nl_addr_get_binary_addr(nl_addr) : NULL;
    int paddr_len = nl_addr ? nl_addr_get_len(nl_addr) : 0;
    unsigned int link_flags;

    free(entry->paddr);
    entry->paddr = netsnmp_memdup(paddr, paddr_len);
    entry->paddr_len = paddr_len;
    entry->type = netsnmp_convert_arphrd_type(rtnl_link_get_arptype(rtnl_link),
                                              rtnl_link_get_type(rtnl_link));
    if (entry->type == 0)
        netsnmp_guess_interface_type(entry);
    netsnmp_derive_interface_id(entry);
    /* IFF_* flags */
    link_flags = rtnl_link_get_flags(rtnl_link);
    netsnmp_process_link_flags(entry, link_flags);
    /* MTU */
    entry->mtu = rtnl_link_get_mtu(rtnl_link);
    /* link speed */
    netsnmp_retrieve_link_speed(fd, entry);

    /*
     * Zero speed means link problem - I'm not sure this is always true.
     */
    if (entry->speed == 0 && entry->os_flags & IFF_UP)
        entry->os_flags &= ~IFF_RUNNING;

    /*
     * check for promiscuous mode.
     *  NOTE: there are 2 ways to set promiscuous mode in Linux
     *  (kernels later than 2.2.something) - using ioctls and
     *  using setsockopt. The ioctl method tested here does not
     *  detect if an interface was set using setsockopt. google
     *  on IFF_PROMISC and linux to see lots of arguments about it.
     */
    if (entry->os_flags & IFF_PROMISC)
        entry->promiscuous = 1; /* boolean */

    /*
     * Hardcoded maximum packet size. See also the following code in the Linux
     * kernel function ip_frag_reasm(): if (len > 65535) goto out_oversize;
     */
    entry->reasm_max_v4 = entry->reasm_max_v6 = 65535;
    entry->ns_flags |=
        NETSNMP_INTERFACE_FLAGS_HAS_V4_REASMMAX |
        NETSNMP_INTERFACE_FLAGS_HAS_V6_REASMMAX;

    netsnmp_access_interface_entry_overrides(entry);

    if (load_stats)
        _retrieve_stats(entry, rtnl_link);

    _arch_interface_flags_v4_get(entry);

#ifdef NETSNMP_ENABLE_IPV6
    _arch_interface_flags_v6_get(entry);
#endif /* NETSNMP_ENABLE_IPV6 */
}

/*
 * Iterate over all network interfaces (links) and store information about
 * the network interfaces in @container.
 */
static void netsnmp_retrieve_link_info(struct nl_sock *nl_sock, int fd,
                                       netsnmp_container *container)
{
    struct nl_cache *link_cache;
    struct nl_object *nl_object;
    int ret;

    ret = rtnl_link_alloc_cache(nl_sock, AF_UNSPEC, &link_cache);
    if (ret)
        return;
#if 0
    {
        // Debugging code: dump the statistics in ASCII format.
        char buf[16384];
        struct nl_dump_params dump_params = {
            .dp_type = NL_DUMP_STATS,
            .dp_buf = buf,
            .dp_buflen = sizeof(buf)
        };
        nl_cache_dump(link_cache, &dump_params);
        write(1, buf, strlen(buf));
    }
#endif
    for (nl_object = nl_cache_get_first(link_cache); nl_object;
         nl_object = nl_cache_get_next(nl_object)) {
        struct rtnl_link *rtnl_link = (void *)nl_object;
        int if_index = rtnl_link_get_ifindex(rtnl_link);
        const char *ifname = rtnl_link_get_name(rtnl_link);
        netsnmp_interface_entry *entry;

        netsnmp_assert(if_index > 0);
        if (if_index <= 0)
            continue;
	if (!netsnmp_access_interface_include(ifname))
            continue;
        entry = netsnmp_access_interface_entry_create(ifname, if_index);
        if (!entry)
            continue;
#ifdef HAVE_PCI_LOOKUP_NAME
	_arch_interface_description_get(entry);
#endif
        netsnmp_retrieve_one_link_info(rtnl_link, fd, entry, TRUE);
        ret = CONTAINER_INSERT(container, entry);
        netsnmp_assert(ret == 0);
    }
    nl_cache_put(link_cache);
}

/* Iterate over all network addresses and set ns_flags in @container. */
static void netsnmp_retrieve_addr_info(struct nl_sock *nl_sock,
                                       netsnmp_container *container)
{
    struct nl_cache *addr_cache;
    struct nl_object *nl_object;
    int ret;

    ret = rtnl_addr_alloc_cache(nl_sock, &addr_cache);
    if (ret)
        return;
    for (nl_object = nl_cache_get_first(addr_cache); nl_object;
         nl_object = nl_cache_get_next(nl_object)) {
        struct rtnl_addr *rtnl_addr = (struct rtnl_addr *)nl_object;
        struct nl_addr *local_addr = rtnl_addr_get_local(rtnl_addr);
        const int if_index = rtnl_addr_get_ifindex(rtnl_addr);
        oid oid_array[1] = { if_index };
        netsnmp_index oid_index = { 1, oid_array };
        netsnmp_interface_entry *entry = CONTAINER_FIND(container, &oid_index);

        if (!entry)
            continue;

        switch (nl_addr_get_family(local_addr)) {
        case AF_INET:
            entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_IPV4;
            break;
        case AF_INET6:
            entry->ns_flags |= NETSNMP_INTERFACE_FLAGS_HAS_IPV6;
            break;
        }
    }
    nl_cache_put(addr_cache);
}

/**
 * Retrieve network interface information.
 *
 * @param container:  Container to store network information in.
 * @param load_flags: One or more NETSNMP_ACCESS_INTERFACE_LOAD_* flags.
 *
 * @retval  0 success
 * @retval -1 no container specified
 * @retval -2 could not retrieve network interface information from OS
 * @retval -3 could not create entry (probably malloc)
 */
int
netsnmp_arch_interface_container_load(netsnmp_container* container,
                                      u_int load_flags)
{
    struct nl_sock *nl_sock;
    int fd, ret;

    DEBUGMSGTL(("access:interface:container:arch", "load (flags %x)\n",
                load_flags));

    if (NULL == container) {
        snmp_log(LOG_ERR, "no container specified/found for interface\n");
        return -1;
    }

    /*
     * create socket for ioctls
     */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
        snmp_log_perror("interface_linux: could not create socket");
        return -2;
    }

    nl_sock = nl_socket_alloc();
    if (!nl_sock) {
        snmp_log(LOG_ERR, "Failed to initialize netlink library\n");
        ret = -2;
        goto close_fd;
    }

    ret = nl_connect(nl_sock, NETLINK_ROUTE);
    if (ret) {
        snmp_log(LOG_ERR, "Failed to read network interface information\n");
        ret = -2;
        goto free_nl_sock;
    }

    netsnmp_retrieve_link_info(nl_sock, fd, container);
    netsnmp_retrieve_addr_info(nl_sock, container);

    ret = 0;

free_nl_sock:
    nl_socket_free(nl_sock);

close_fd:
    close(fd);

    return ret;
}

#ifndef NETSNMP_FEATURE_REMOVE_INTERFACE_ARCH_SET_ADMIN_STATUS
int
netsnmp_arch_set_admin_status(netsnmp_interface_entry * entry,
                              int ifAdminStatus_val)
{
    int and_complement;
    
    DEBUGMSGTL(("access:interface:arch", "set_admin_status\n"));

    if(IFADMINSTATUS_UP == ifAdminStatus_val)
        and_complement = 0; /* |= */
    else
        and_complement = 1; /* &= ~ */

    return netsnmp_access_interface_ioctl_flags_set(-1, entry,
                                                    IFF_UP, and_complement);
}
#endif /* NETSNMP_FEATURE_REMOVE_INTERFACE_ARCH_SET_ADMIN_STATUS */

#ifdef HAVE_LINUX_ETHTOOL_H
/**
 * Determines network interface speed from ETHTOOL_GLINKSETTINGS
 * In case of failure revert to obsolete ETHTOOL_GSET
 */
unsigned long long
netsnmp_linux_interface_get_if_speed(int fd, const char *name,
            unsigned long long defaultspeed)
{
    int ret;
    struct netsnmp_linux_link_settings nlls;
    uint32_t speed = -1;

    ret = netsnmp_get_link_settings(&nlls, fd, name);
    if (ret < 0) {
        DEBUGMSGTL(("mibII/interfaces", "ETHTOOL_GSET on %s failed (%d / %d)\n",
                    name, ret, speed));
        return netsnmp_linux_interface_get_if_speed_mii(fd, name, defaultspeed);
    }
    speed = nlls.speed;
    if (speed == 0xffff || speed == 0xffffffffUL /*SPEED_UNKNOWN*/)
        speed = defaultspeed;
    /* return in bps */
    DEBUGMSGTL(("mibII/interfaces", "ETHTOOL_GSET on %s speed = %#x = %d\n",
                name, speed, speed));
    return speed * 1000LL * 1000LL;
}
#endif
 
/**
 * Determines network interface speed from MII
 */
unsigned long long
#ifdef HAVE_LINUX_ETHTOOL_H
netsnmp_linux_interface_get_if_speed_mii(int fd, const char *name,
        unsigned long long  defaultspeed)
#else
netsnmp_linux_interface_get_if_speed(int fd, const char *name,
        unsigned long long defaultspeed)
#endif
{
    unsigned long long retspeed = defaultspeed;
    struct ifreq ifr;

    /* the code is based on mii-diag utility by Donald Becker
     * see ftp://ftp.scyld.com/pub/diag/mii-diag.c
     */
    ushort *data = (ushort *)(&ifr.ifr_data);
    unsigned phy_id;
    int mii_reg, i;
    ushort mii_val[32];
    ushort bmcr, bmsr, nway_advert, lkpar;
    const unsigned long long media_speeds[] = {10000000, 10000000, 100000000, 100000000, 10000000, 0};
    /* It corresponds to "10baseT", "10baseT-FD", "100baseTx", "100baseTx-FD", "100baseT4", "Flow-control", 0, */

    strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
    data[0] = 0;
    
    /*
     * SIOCGMIIPHY has been defined since at least kernel 2.4.10 (Sept 2001).
     * It's probably safe to drop the interim SIOCDEVPRIVATE handling now!
     */
    if (ioctl(fd, SIOCGMIIPHY, &ifr) < 0) {
        DEBUGMSGTL(("mibII/interfaces", "SIOCGMIIPHY on %s failed\n",
                    ifr.ifr_name));
        return retspeed;
    }

    /* Begin getting mii register values */
    phy_id = data[0];
    for (mii_reg = 0; mii_reg < 8; mii_reg++){
        data[0] = phy_id;
        data[1] = mii_reg;
        if(ioctl(fd, SIOCGMIIREG, &ifr) <0){
            DEBUGMSGTL(("mibII/interfaces", "SIOCGMIIREG on %s failed\n", ifr.ifr_name));
        }
        mii_val[mii_reg] = data[3];		
    }
    /*Parsing of mii values*/
    /*Invalid basic mode control register*/
    if (mii_val[0] == 0xffff  ||  mii_val[1] == 0x0000) {
        DEBUGMSGTL(("mibII/interfaces", "No MII transceiver present!.\n"));
        return retspeed;
    }
    /* Descriptive rename. */
    bmcr = mii_val[0]; 	  /*basic mode control register*/
    bmsr = mii_val[1]; 	  /* basic mode status register*/
    nway_advert = mii_val[4]; /* autonegotiation advertisement*/
    lkpar = mii_val[5]; 	  /*link partner ability*/
    
    /*Check for link existence, returns 0 if link is absent*/
    if ((bmsr & 0x0016) != 0x0004){
        DEBUGMSGTL(("mibII/interfaces", "No link...\n"));
        retspeed = 0;
        return retspeed;
    }
    
    if(!(bmcr & 0x1000) ){
        DEBUGMSGTL(("mibII/interfaces", "Auto-negotiation disabled.\n"));
        retspeed = bmcr & 0x2000 ? 100000000 : 10000000;
        return retspeed;
    }
    /* Link partner got our advertised abilities */	
    if (lkpar & 0x4000) {
        int negotiated = nway_advert & lkpar & 0x3e0;
        int max_capability = 0;
        /* Scan for the highest negotiated capability, highest priority
           (100baseTx-FDX) to lowest (10baseT-HDX). */
        int media_priority[] = {8, 9, 7, 6, 5}; 	/* media_names[i-5] */
        for (i = 0; media_priority[i]; i++){
            if (negotiated & (1 << media_priority[i])) {
                max_capability = media_priority[i];
                break;
            }
        }
        if (max_capability)
            retspeed = media_speeds[max_capability - 5];
        else
            DEBUGMSGTL(("mibII/interfaces", "No common media type was autonegotiated!\n"));
    }else if(lkpar & 0x00A0){
        retspeed = (lkpar & 0x0080) ? 100000000 : 10000000;
    }
    return retspeed;
}
#ifdef SUPPORT_PREFIX_FLAGS
void netsnmp_prefix_process(int fd, void *data);

/* Open netlink socket to watch new ipv6 addresses and prefixes. */
int netsnmp_prefix_listen(void)
{
    struct {
                struct nlmsghdr n;
                struct ifinfomsg r;
                char   buf[1024];
    } req;

    struct rtattr      *rta;
    int                status;
    struct sockaddr_nl localaddrinfo;
    unsigned           groups = 0;

    int fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (fd < 0) {
        snmp_log(LOG_ERR, "netsnmp_prefix_listen: Cannot create socket.\n");
        return -1;
    }

    memset(&localaddrinfo, 0, sizeof(struct sockaddr_nl));

    groups |= RTMGRP_IPV6_IFADDR;
    groups |= RTMGRP_IPV6_PREFIX;
    localaddrinfo.nl_family = AF_NETLINK;
    localaddrinfo.nl_groups = groups;

    if (bind(fd, (struct sockaddr*)&localaddrinfo, sizeof(localaddrinfo)) < 0) {
        snmp_log(LOG_ERR,"netsnmp_prefix_listen: Bind failed.\n");
        close(fd);
        return -1;
    }

    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
    req.n.nlmsg_type = RTM_GETLINK;
    req.r.ifi_family = AF_INET6;
    rta = (struct rtattr *)(((char *)&req) + NLMSG_ALIGN(req.n.nlmsg_len));
    rta->rta_len = RTA_LENGTH(16);

    status = send(fd, &req, req.n.nlmsg_len, 0);
    if (status < 0) {
        snmp_log(LOG_ERR,"netsnmp_prefix_listen: send failed\n");
        close(fd);
        return -1;
    }

    if (register_readfd(fd, netsnmp_prefix_process, NULL) != 0) {
        snmp_log(LOG_ERR,"netsnmp_prefix_listen: error registering netlink socket\n");
        close(fd);
        return -1;
    }
    return 0;
}

/* Process one incoming netlink packets.
 * RTM_NEWADDR and RTM_NEWPREFIX usually arrive in separate packets
 * -> information from these packets must be stored locally and
 * new prefix is added when information from both packets is complete.
 */
void netsnmp_prefix_process(int fd, void *data)
{
    int                status;
    char               buf[16384];
    struct nlmsghdr    *nlmp;
    struct rtattr      *rtatp;
    struct ifaddrmsg   *ifa;
    struct prefixmsg   *prefix;
    struct in6_addr    *in6p;

    /* these values must persist between calls */
    static char               in6pAddr[40];
    static int                have_addr = 0,have_prefix = 0;
    static int                onlink = 2,autonomous = 2; /*Assume as false*/

    int                iret;
    prefix_cbx         *new;
    int                len, req_len, length; 

    status = recv(fd, buf, sizeof(buf), 0);
    if (status < 0) {
        if (errno == EINTR)
            return;
        snmp_log(LOG_ERR,"netsnmp_prefix_listen: Receive failed.\n");
        return;
    }

    if(status == 0){
        DEBUGMSGTL(("access:interface:prefix", "End of File\n"));
        return;
    }

    for(nlmp = (struct nlmsghdr *)buf; status > sizeof(*nlmp);){
        len = nlmp->nlmsg_len;
        req_len = len - sizeof(*nlmp);

        if (req_len < 0 || len > status) {
            snmp_log(LOG_ERR,"netsnmp_prefix_listen: Error in length.\n");
            return;
        }

        if (!NLMSG_OK(nlmp, status)) {
            DEBUGMSGTL(("access:interface:prefix", "NLMSG not OK\n"));
            continue;
        }

        if (nlmp->nlmsg_type == RTM_NEWADDR || nlmp->nlmsg_type == RTM_DELADDR) {
            ifa = NLMSG_DATA(nlmp);
            length = nlmp->nlmsg_len;
            length -= NLMSG_LENGTH(sizeof(*ifa));

            if (length < 0) {
                DEBUGMSGTL(("access:interface:prefix", "wrong nlmsg length %d\n", length));
                continue;
            }

            if(!ifa->ifa_flags) {
                rtatp = IFA_RTA(ifa);
                while (RTA_OK(rtatp, length)) {
                    if (rtatp->rta_type == IFA_ADDRESS){
                        in6p = (struct in6_addr *) RTA_DATA(rtatp);
                        if(nlmp->nlmsg_type == RTM_DELADDR) {
                            snprintf(in6pAddr, sizeof(in6pAddr), "%04x%04x%04x%04x%04x%04x%04x%04x", NIP6(*in6p));
                            have_addr = -1;
                            break;
                        } else {
                            snprintf(in6pAddr, sizeof(in6pAddr), "%04x%04x%04x%04x%04x%04x%04x%04x", NIP6(*in6p));
                            have_addr = 1;
                            break;
                        }
                    }
                    rtatp = RTA_NEXT(rtatp,length);
                }
            }
        }

        if(nlmp->nlmsg_type == RTM_NEWPREFIX) {
            prefix = NLMSG_DATA(nlmp);
            length = nlmp->nlmsg_len;
            length -= NLMSG_LENGTH(sizeof(*prefix));

            if (length < 0) {
                DEBUGMSGTL(("access:interface:prefix", "wrong nlmsg length %d\n", length));
                continue;
            }
            have_prefix = 1;
            if (prefix->prefix_flags & IF_PREFIX_ONLINK) {
                onlink = 1; 
            }
            if (prefix->prefix_flags & IF_PREFIX_AUTOCONF) {
                autonomous = 1;
            }
        }
        status -= NLMSG_ALIGN(len);
        nlmp = (struct nlmsghdr*)((char*)nlmp + NLMSG_ALIGN(len));
    }

    if((have_addr == 1) && (have_prefix == 1)){
        if(!(new = net_snmp_create_prefix_info (onlink, autonomous, in6pAddr)))
            DEBUGMSGTL(("access:interface:prefix", "Unable to create prefix info\n"));
        else {

            iret = net_snmp_search_update_prefix_info (list_info.list_head, new, 0);
            if(iret < 0) {
                DEBUGMSGTL(("access:interface:prefix", "Unable to add/update prefix info\n"));
                free(new);
            }
            if(iret == 2) /*Only when entry already exists and we are only updating*/
                free(new);
        }
        have_addr = have_prefix = 0;
        onlink = autonomous = 2; /*Set to defaults again*/
    } else if (have_addr == -1) {
        iret = net_snmp_delete_prefix_info (list_info.list_head, in6pAddr);
        if(iret < 0)
            DEBUGMSGTL(("access:interface:prefix", "Unable to delete the prefix info\n"));
        if(!iret)
            DEBUGMSGTL(("access:interface:prefix", "Unable to find the node to delete\n"));
        have_addr = 0;
    }
}
#endif

