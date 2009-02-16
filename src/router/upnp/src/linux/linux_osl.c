/*
 * Broadcom UPnP module linux OS dependent implementation
 *
 * Copyright (C) 2008, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: linux_osl.c,v 1.14 2008/10/27 08:37:43 Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>

#define __KERNEL__
#include <asm/types.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>

#include "shutils.h"
#include "utils.h"
#include "nvparse.h"
#include "netconf.h"
#include "upnp.h"
#include <InternetGatewayDevice.h>
#include <wlutils.h>

#define _PATH_PROCNET_DEV           "/proc/net/dev"

static int get_lan_mac(unsigned char *mac);


extern char g_wandevs[];

/*
 * The following functions are required by the
 * upnp engine, which the OSL has to implement.
 * Most of them are things about accessing the
 * NVRAM.
 */
int
upnp_osl_igd_status()
{
	return !nvram_match("router_disable", "1");
}

int
upnp_osl_primary_lanmac(char *mac)
{
	return get_lan_mac(mac);
}

int
upnp_osl_read_config(UPNP_CONFIG *config)
{
	char *value;

	memset(config, 0, sizeof(*config));

	/* Get OS/VER */
	strcpy(config->os_name, "DD-WRT Linux");

	strcpy(config->os_ver, "V24");

	/* initialize upnp_config to default values */
	if ((value = nvram_get("upnp_port")))
		config->http_port = atoi(value);
	else
		config->http_port = 1780;

	if ((value = nvram_get("upnp_ad_time")))
		config->adv_time = atoi(value);
	else
		config->adv_time = 300;

	if ((value = nvram_get("upnp_sub_timeout")))
		config->sub_time = atoi(value);
	else
		config->sub_time = 300;

	return 0;
}

int
upnp_osl_ifname_list(char *ifname_list)
{
	int count;
	char *value;

	/* Null end the string */
	ifname_list[0] = 0;
	char buf[512];
	memset(buf,0,512);
	int cnt = getIfList(buf,NULL);
	int i=0;
	char *p=&buf[0];
	/* Cat all lan interface together */
	for (count = 0; count < 255; count++) {
		char name[IFNAMSIZ];
		char buf[IFNAMSIZ + 8];

		if (count == 0)
			{
			strcpy(name, "lan_ifname");
			value = nvram_get(name);
			}
		else
		    {
		    again:;
		    if (i==cnt)
			return 0;
		    value = p;
		    if (i<cnt-1)
		    {
		    p=strstr(p," ");
		    p[0]=0;
		    p++;
		    }
		    i++;
		
		    if (nvram_match("lan_ifname",value))
			goto again;
		    if (nvram_nmatch("1","%s_bridged",value))
			goto again;
		    }
		if (value) {
			if (ifname_list[0] != 0)
				strcat(ifname_list, " ");

			/*
			 * For example, the config->ifnames will be
			 * 0=br0 1=br1 ...
			 */
			sprintf(buf, "%d=%s", count, value);
			strcat(ifname_list, buf);
		}
	}
	return 0;
}

int
upnp_osl_ifaddr(const char *ifname, struct in_addr *inaddr)
{
	int sockfd;
	struct ifreq ifreq;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0) {
		close(sockfd);
		return -1;
	}
	else {
		memcpy(inaddr, &(((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr),
			sizeof(struct in_addr));
	}

	close(sockfd);
	return 0;
}

int
upnp_osl_netmask(const char *ifname, struct in_addr *inaddr)
{
	int sockfd;
	struct ifreq ifreq;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(sockfd, SIOCGIFNETMASK, &ifreq) < 0) {
		close(sockfd);
		return -1;
	}
	else {
		memcpy(inaddr, &(((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr),
			sizeof(struct in_addr));
	}

	close(sockfd);
	return 0;
}

int
upnp_osl_hwaddr(const char *ifname, char *mac)
{
	int sockfd;
	struct ifreq  ifreq;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return -1;
	}

	strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(sockfd, SIOCGIFHWADDR, &ifreq) < 0) {
		close(sockfd);
		return -1;
	}
	else {
		memcpy(mac, ifreq.ifr_hwaddr.sa_data, 6);
	}

	close(sockfd);
	return 0;
}

/* Create a udp socket with a specific ip and port */
int
upnp_open_udp_socket(struct in_addr addr, unsigned short port)
{
	int s;
	int reuse = 1;
	struct sockaddr_in sin;

	/* create UDP socket */
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		return -1;

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		upnp_syslog(LOG_ERR, "Cannot set socket option (SO_REUSEPORT)");
	}

	/* bind socket to recive discovery */
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr = addr;

	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		upnp_syslog(LOG_ERR, "bind failed!");
		close(s);
		return -1;
	}

	return s;
}

/* Create a tcp socket with a specific ip and port */
int
upnp_open_tcp_socket(struct in_addr addr, unsigned short port)
{
	int s;
	int reuse = 1;
	struct sockaddr_in sin;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return -1;

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		upnp_syslog(LOG_ERR, "Cannot set socket option (SO_REUSEPORT)");
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr = addr;

	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0 ||
	    listen(s, MAX_WAITS) < 0) {
		close(s);
		return -1;
	}

	return s;
}

/*
 * The functions below are required by the
 * upnp device, for example, InternetGatewayDevice.
 */
/* WAN specific settings */
char *
igd_pri_wan_var(char *prefix, int len, char *var)
{
	int unit;
	char tmp[100];

	for (unit = 0; unit < 10; unit ++) {
		snprintf(tmp, sizeof(tmp), "wan%d_primary", unit);
		if (nvram_match(tmp, "1"))
			break;
	}
	if (unit == 10)
		unit = 0;

	snprintf(prefix, len, "wan%d_%s", unit, var);
	return (prefix);
}

char *
get_name(char *name, char *p)
{
	while (isspace(*p))
		p++;

	while (*p) {
		/* Eat white space */
		if (isspace(*p))
			break;

		/* could be an alias */
		if (*p == ':') {
			char *dot = p, *dotname = name;
			*name++ = *p++;
			while (isdigit(*p))
				*name++ = *p++;

			/* it wasn't, backup */
			if (*p != ':') {
				p = dot;
				name = dotname;
			}
			if (*p == '\0')
				return NULL;

			p++;
			break;
		}

		*name++ = *p++;
	}

	*name++ = '\0';
	return p;
}

int
procnetdev_version(char *buf)
{
	if (strstr(buf, "compressed"))
		return 3;

	if (strstr(buf, "bytes"))
		return 2;

	return 1;
}

int
get_dev_fields(char *bp, int versioninfo, if_stats_t *pstats)
{
	switch (versioninfo) {
	case 3:
		sscanf(bp, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
			&pstats->rx_bytes,
			&pstats->rx_packets,
			&pstats->rx_errors,
			&pstats->rx_dropped,
			&pstats->rx_fifo_errors,
			&pstats->rx_frame_errors,
			&pstats->rx_compressed,
			&pstats->rx_multicast,
			&pstats->tx_bytes,
			&pstats->tx_packets,
			&pstats->tx_errors,
			&pstats->tx_dropped,
			&pstats->tx_fifo_errors,
			&pstats->collisions,
			&pstats->tx_carrier_errors,
			&pstats->tx_compressed);
		break;

	case 2:
		sscanf(bp, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
			&pstats->rx_bytes,
			&pstats->rx_packets,
			&pstats->rx_errors,
			&pstats->rx_dropped,
			&pstats->rx_fifo_errors,
			&pstats->rx_frame_errors,
			&pstats->tx_bytes,
			&pstats->tx_packets,
			&pstats->tx_errors,
			&pstats->tx_dropped,
			&pstats->tx_fifo_errors,
			&pstats->collisions,
			&pstats->tx_carrier_errors);

		pstats->rx_multicast = 0;
		break;

	case 1:
		sscanf(bp, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
			&pstats->rx_packets,
			&pstats->rx_errors,
			&pstats->rx_dropped,
			&pstats->rx_fifo_errors,
			&pstats->rx_frame_errors,
			&pstats->tx_packets,
			&pstats->tx_errors,
			&pstats->tx_dropped,
			&pstats->tx_fifo_errors,
			&pstats->collisions,
			&pstats->tx_carrier_errors);

		pstats->rx_bytes = 0;
		pstats->tx_bytes = 0;
		pstats->rx_multicast = 0;
		break;
	}

	return 0;
}

int
upnp_osl_wan_ifstats(if_stats_t *pstats)
{
	char *ifname = g_wandevs;

	extern int get_dev_fields(char *, int, if_stats_t *);
	extern int procnetdev_version(char *);
	extern char *get_name(char *, char *);

	FILE *fh;
	char buf[512];
	int err;
	int procnetdev_vsn;  /* version information */

	memset(pstats, 0, sizeof(*pstats));

	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh) {
		fprintf(stderr, "Warning: cannot open %s (%s). Limited output.\n",
			_PATH_PROCNET_DEV, strerror(errno));
		return 0;
	}

	fgets(buf, sizeof(buf), fh);	/* eat line */
	fgets(buf, sizeof(buf), fh);

	procnetdev_vsn = procnetdev_version(buf);

	err = 0;
	while (fgets(buf, sizeof(buf), fh)) {
		char *s;
		char name[50];

		s = get_name(name, buf);
		if (strcmp(name, ifname) == 0) {
			get_dev_fields(s, procnetdev_vsn, pstats);
			break;
		}
	}
	if (ferror(fh)) {
		perror(_PATH_PROCNET_DEV);
		err = -1;
	}
	fclose(fh);

	return err;
}

int
upnp_osl_wan_link_status()
{
	char *devname = g_wandevs;

	struct ifreq ifr;
	int fd, err;
	uint if_up = 0;
	struct ethtool_cmd ecmd;

	/* Setup our control structures. */
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, devname);

	/* Open control socket. */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd >= 0) {
		ecmd.cmd = ETHTOOL_GSET;
		ifr.ifr_data = (caddr_t)&ecmd;

		err = ioctl(fd, SIOCETHTOOL, &ifr);
		if (err >= 0) {
			switch (ecmd.speed) {
			case SPEED_10:
			case SPEED_100:
			case SPEED_1000:
				if_up = 1;
				break;
			}
		}

		/* close the control socket */
		close(fd);
	}

	return if_up;
}

unsigned int
upnp_osl_wan_max_bitrates(unsigned long *rx, unsigned long *tx)
{
	char *devname = g_wandevs;

	struct ethtool_cmd ecmd;
	struct ifreq ifr;
	int fd, err;
	long speed = 0;

	/* Setup our control structures. */
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, devname);

	/* Open control socket. */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd >= 0) {
		ecmd.cmd = ETHTOOL_GSET;
		ifr.ifr_data = (caddr_t)&ecmd;

		err = ioctl(fd, SIOCETHTOOL, &ifr);
		if (err >= 0) {

			unsigned int mask = ecmd.supported;

			/* dump_ecmd(&ecmd); */
			if ((mask & (ADVERTISED_1000baseT_Half|ADVERTISED_1000baseT_Full))) {
				speed = (1000 * 1000000);
			}
			else if ((mask & (ADVERTISED_100baseT_Half|ADVERTISED_100baseT_Full))) {
				speed = (100 * 1000000);
			}
			else if ((mask & (ADVERTISED_10baseT_Half|ADVERTISED_10baseT_Full))) {
				speed = (10 * 1000000);
			}
			else {
				speed = 0;
			}
		}
		else {
			upnp_syslog(LOG_INFO, "ioctl(SIOCETHTOOL) failed");
		}

		/* close the control socket */
		close(fd);
	}
	else {
		upnp_syslog(LOG_INFO, "cannot open socket");
	}

	*rx = *tx = speed;
	return TRUE;
}

int
upnp_osl_wan_ip(struct in_addr *inaddr)
{
	char tmp[32];
	int status = 0;

	inaddr->s_addr = 0;

	if (strcasecmp(nvram_safe_get(igd_pri_wan_var(tmp, sizeof(tmp), "proto")),
		"disabled") != 0) {
		if (upnp_osl_ifaddr(nvram_safe_get(igd_pri_wan_var(tmp,
			sizeof(tmp), "ifname")), inaddr) == 0) {
			if (inaddr->s_addr != 0) {
				status = 1;
			}
		}
	}

	return status;
}

int
upnp_osl_wan_isup()
{
	struct in_addr inaddr = {0};
	int  status = 0;
	char tmp[100];

	if (strcasecmp(nvram_safe_get(igd_pri_wan_var(tmp, sizeof(tmp), "proto")),
		"disabled") != 0) {
		if (upnp_osl_ifaddr(nvram_safe_get(igd_pri_wan_var(tmp,
			sizeof(tmp), "ifname")), &inaddr) == 0) {
			if (inaddr.s_addr != 0) {
				status = 1;
			}
		}
	}

	return status;
}

int
upnp_osl_wan_uptime()
{
	FILE *fp;
	long int second1, second2;

	fp = fopen("/proc/uptime", "r");
	if (!fp) {
		fprintf(stderr, "can't open \"uptime\" !\n");
		return -1;
	}

	fscanf(fp, "%ld.%ld", &second1, &second2);
	fclose(fp);

	return second1;
}

/*
 * UPnP portmapping --
 * Add port forward and a matching ACCEPT rule to the FORWARD table
 */
static void
add_nat_entry(netconf_nat_t *entry)
{
	int dir = NETCONF_FORWARD;
	int log_level = atoi(nvram_safe_get("log_level"));
	int target = (log_level & 2) ? NETCONF_LOG_ACCEPT : NETCONF_ACCEPT;
	netconf_filter_t filter;
	struct in_addr netmask = { 0xffffffff };
	netconf_nat_t nat = *entry;

	if (entry->ipaddr.s_addr == 0xffffffff) {
		inet_aton(nvram_safe_get("lan_ipaddr"), &nat.ipaddr);
		inet_aton(nvram_safe_get("lan_netmask"), &netmask);
		nat.ipaddr.s_addr &= netmask.s_addr;
		nat.ipaddr.s_addr |= (0xffffffff & ~netmask.s_addr);
	}

	/* Set up LAN side match */
	memset(&filter, 0, sizeof(filter));
	filter.match.ipproto = nat.match.ipproto;
	filter.match.src.ports[1] = nat.match.src.ports[1];
	filter.match.dst.ipaddr.s_addr = nat.ipaddr.s_addr;
	filter.match.dst.netmask.s_addr = netmask.s_addr;
	filter.match.dst.ports[0] = nat.ports[0];
	filter.match.dst.ports[1] = nat.ports[1];
	strncpy(filter.match.in.name, nat.match.in.name, IFNAMSIZ);

	/* Accept connection */
	filter.target = target;
	filter.dir = dir;
	set_forward_port(&nat);
	/* Do it */
	netconf_add_nat(&nat);
	netconf_add_filter(&filter);
}

/* Combination PREROUTING DNAT and FORWARD ACCEPT */
static void
delete_nat_entry(netconf_nat_t *entry)
{
	int dir = NETCONF_FORWARD;
	int log_level = atoi(nvram_safe_get("log_level"));
	int target = (log_level & 2) ? NETCONF_LOG_ACCEPT : NETCONF_ACCEPT;
	netconf_filter_t filter;
	struct in_addr netmask = { 0xffffffff };
	netconf_nat_t nat = *entry;

	if (entry->ipaddr.s_addr == 0xffffffff) {
		inet_aton(nvram_safe_get("lan_ipaddr"), &nat.ipaddr);
		inet_aton(nvram_safe_get("lan_netmask"), &netmask);
		nat.ipaddr.s_addr &= netmask.s_addr;
		nat.ipaddr.s_addr |= (0xffffffff & ~netmask.s_addr);
	}

	/* Set up LAN side match */
	memset(&filter, 0, sizeof(filter));
	filter.match.ipproto = nat.match.ipproto;
	filter.match.src.ports[1] = nat.match.src.ports[1];
	filter.match.dst.ipaddr.s_addr = nat.ipaddr.s_addr;
	filter.match.dst.netmask.s_addr = netmask.s_addr;
	filter.match.dst.ports[0] = nat.ports[0];
	filter.match.dst.ports[1] = nat.ports[1];
	strncpy(filter.match.in.name, nat.match.in.name, IFNAMSIZ);

	/* Accept connection */
	filter.target = target;
	filter.dir = dir;

	del_forward_port(&nat);
	/* Do it */
	errno = netconf_del_nat(&nat);
	if (errno)
		upnp_syslog(LOG_INFO, "netconf_del_nat returned error %d\n", errno);

	errno = netconf_del_filter(&filter);
	if (errno)
		upnp_syslog(LOG_INFO, "netconf_del_filter returned error %d\n", errno);
}

/* Command to add or delete from NAT engine */
void
upnp_osl_nat_config(UPNP_PORTMAP *map)
{
	netconf_nat_t nat, *entry = &nat;

	char *Proto = map->protocol;
	unsigned short ExtPort = map->external_port;
	char *IntIP = map->internal_client;
	unsigned short IntPort = map->internal_port;
	char *rthost = map->remote_host;

	memset(entry, 0, sizeof(netconf_nat_t));

	/* accept from any port */
	entry->match.src.ports[0] = 0;
	entry->match.src.ports[1] = htons(0xffff);
	entry->match.dst.ports[0] = htons(ExtPort);
	entry->match.dst.ports[1] = htons(ExtPort);

	if (strlen(rthost)) {
		inet_aton("255.255.255.255", &entry->match.dst.netmask);
		inet_aton(rthost, &entry->match.dst.ipaddr);
	}

	/* parse the specification of the internal NAT client. */
	entry->target = NETCONF_DNAT;

	if (IntIP && IntPort != 0) {
		/* parse the internal ip address. */
		inet_aton(IntIP, (struct in_addr *)&entry->ipaddr);

		/* parse the internal port number */
		entry->ports[0] = htons(IntPort);
		entry->ports[1] = htons(IntPort);
	}

	if (strcasecmp(Proto, "TCP") == 0) {
		entry->match.ipproto = IPPROTO_TCP;
	}
	else if (strcasecmp(Proto, "UDP") == 0) {
		entry->match.ipproto = IPPROTO_UDP;
	}
	
	strncpy(entry->desc, map->description, sizeof(entry->desc));

	/* set to NAT kernel */
	if (map->enable) {
		entry->match.flags &= ~NETCONF_DISABLED;
		add_nat_entry(entry);
	}
	else {
		entry->match.flags |= NETCONF_DISABLED;
		delete_nat_entry(entry);
	}
	return;
}

void
upnp_osl_update_wfa_subc_num(int if_instance, int num)
{
	char *upnp_subc_num, prefix[32];
	char num_string[8];

	if (if_instance == 0)
		snprintf(prefix, sizeof(prefix), "lan_upnp_wfa_subc_num");
	else
		snprintf(prefix, sizeof(prefix), "lan%d_upnp_wfa_subc_num", if_instance);

	upnp_subc_num = nvram_get(prefix);
	if (!upnp_subc_num || (num != atoi(upnp_subc_num))) {
		snprintf(num_string, sizeof(num_string), "%d", num);
		nvram_set(prefix, num_string);
	}

	return;
}

static int
get_lan_mac(unsigned char *mac)
{
	unsigned char *lanmac_str = nvram_get("lan_hwaddr");

	if (mac)
		memset(mac, 0, 6);

	if (!lanmac_str || mac == NULL)
		return -1;

	ether_atoe(lanmac_str, mac);

	return 0;
}
