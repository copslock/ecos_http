//==========================================================================
//
//      tests/bridge.c
//
//     
//
//==========================================================================
// ####BSDALTCOPYRIGHTBEGIN####                                             
// -------------------------------------------                              
// Portions of this software may have been derived from FreeBSD, OpenBSD,   
// or other sources, and if so are covered by the appropriate copyright     
// and license included herein.                                             
// -------------------------------------------                              
// ####BSDALTCOPYRIGHTEND####                                               
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    Jason L. Wright (jason@thought.net)
// Contributors: andrew.lunn@ascom.ch (Andrew Lunn), hmt
// Date:         2000-01-10
// Purpose:      
// Description:  
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================


/*	$OpenBSD: brconfig.c,v 1.6 2000/02/04 06:32:04 deraadt Exp $	*/

/*
 * Copyright (c) 1999, 2000 Jason L. Wright (jason@thought.net)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Jason L. Wright
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <pkgconf/system.h>
#include <pkgconf/devs_eth_rltk_819x_wlan.h>
#include <stdio.h>
#include <network.h>
#include <sys/types.h>
#include <stdlib.h>

#ifdef CYGINT_NET_BRIDGE_HANDLER
#ifdef CONFIG_NET_STACK_FREEBSD
#include <net/if_var.h>
#else
#include <net/if.h>
#endif
#include <net/if_dl.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if_bridge.h>
#include <sys/errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include "net_api.h"
#ifdef HAVE_APMIB
#include "apmib.h"
#endif

void bridge_usage __P((void));
int main __P((int, char **));
int bridge_setflag __P((int, char *, short));
int bridge_clrflag __P((int, char *, short));
int bridge_ifsetflag __P((int, char *, char *, u_int32_t));
int bridge_ifclrflag __P((int, char *, char *, u_int32_t));
int bridge_list __P((int, char *, char *));
int bridge_addrs __P((int, char *, char *));
int bridge_addaddr __P((int, char *, char *, char *));
int bridge_deladdr __P((int, char *, char *));
int bridge_maxaddr __P((int, char *, char *));
int bridge_timeout __P((int, char *, char *));
int bridge_flush __P((int, char *));
int bridge_flushall __P((int, char *));
int bridge_add __P((int, char *, char *));
int bridge_delete __P((int, char *, char *));
int bridge_status __P((int, char *));
int is_bridge __P((int, char *));
int bridge_show_all __P((int));
void printb __P((char *, unsigned short, char *));
int bridge_rule __P((int, char *, int, char **, int));
int bridge_rules __P((int, char *, char *, char *));
int bridge_flush_rule __P((int, char *, char *));
void bridge_badrule __P((int, char **, int));
void bridge_showrule __P((struct ifbrlreq *, char *));
int bridge_rulefile __P((int, char *, char *));

/* if_flags bits: borrowed from ifconfig.c */
#define	IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5POINTOPOINT\6NOTRAILERS\7RUNNING\10NOARP\
\11PROMISC\12ALLMULTI\13OACTIVE\14SIMPLEX\15LINK0\16LINK1\17LINK2\20MULTICAST"

#define	IFBAFBITS	"\020\1STATIC"
#define	IFBIFBITS	"\020\1LEARNING\2DISCOVER\3BLOCKNONIP\4BLOCKARP"

#define warnx(x) printf(x);
#define warnx2(x,y) printf(x,y) 
#define warnx3(x,y,z) printf(x,y,z) 
#define err(x,y) assert(0);
extern  int cyg_kmem_print_stats(void);

static char * _ether_aton __P((char *, struct ether_addr *));

void
bridge_usage()
{
	printf("usage: brconfig -a\n");
	printf(
	    "usage: brconfig interface [up] [down] [add if] [del if] ...\n");
}

int
brconfig_main(unsigned int argc, unsigned char *argv[])
{
	int error = 0, sock;
	char *brdg;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sock >= 0);

	if (argc < 1) {
		bridge_show_all(sock);
		goto done;
	}

	brdg = (char *)argv[0];

	if (strlen(brdg) >= IFNAMSIZ) {
		warnx2("%s is not a bridge\n", brdg);
		goto done;
	}

	if (!is_bridge(sock, brdg)) {
		if (errno == ENXIO)
			warnx2("%s\n", brdg);
		else
			warnx2("%s is not a bridge\n", brdg);
		goto done;
	}

	if (argc == 1) {
		error = bridge_status(sock, brdg);
		goto done;
	}

	for (argc--, argv++; argc != 0; argc--, argv++) {
		if (strcmp("add", (char *)argv[0]) == 0) {
			argc--; argv++;
			if (argc == 0) {
				warnx("add requires an argument\n");
				goto done;
			}
			error = bridge_add(sock, brdg, (char *)argv[0]);
			if (error)
				goto done;
		} else if (strcmp("delete", (char *)argv[0]) == 0 ||
		    strcmp("del", (char *)argv[0]) == 0) {
			argc--; argv++;
			if (argc == 0) {
				warnx("delete requires an argument\n");
				goto done;
			}
			error = bridge_delete(sock, brdg, (char *)argv[0]);
			if (error)
				goto done;
		} else if (strcmp("up", (char *)argv[0]) == 0) {
			error = bridge_setflag(sock, brdg, IFF_UP);
			if (error)
				goto done;
		} else if (strcmp("down", (char *)argv[0]) == 0) {
			error = bridge_clrflag(sock, brdg, IFF_UP);
			if (error)
				goto done;
		} else if (strcmp("flush", (char *)argv[0]) == 0) {
			error = bridge_flush(sock, brdg);
			if (error)
				goto done;
		} else if (strcmp("flushall", (char *)argv[0]) == 0) {
			error = bridge_flushall(sock, brdg);
			if (error)
				goto done;
		} else if (strcmp("static", (char *)argv[0]) == 0) {
			argc--; argv++;
			if (argc < 2) {
				warnx("static requires 2 arguments\n");
				goto done;
			}
			error = bridge_addaddr(sock, brdg, (char *)argv[0], (char *)argv[1]);
			if (error)
				goto done;
			argc--; argv++;
		} else if (strcmp("deladdr", (char *)argv[0]) == 0) {
			argc--; argv++;
			if (argc == 0) {
				warnx("deladdr requires an argument\n");
				goto done;
			}
			error = bridge_deladdr(sock, brdg, (char *)argv[0]);
			if (error)
				goto done;
		} else if (strcmp("addr", (char *)argv[0]) == 0) {
			error = bridge_addrs(sock, brdg, "");
			if (error)
				goto done;
		} else if (strcmp("maxaddr", (char *)argv[0]) == 0) {
			argc--; argv++;
			if (argc == 0) {
				warnx("maxaddr requires an argument\n");
				goto done;
			}
			error = bridge_maxaddr(sock, brdg, (char *)argv[0]);
			if (error)
				goto done;
		} else if (strcmp("timeout", (char *)argv[0]) == 0) {
			argc--; argv++;
			if (argc == 0) {
				warnx("timeout requires an argument\n");
				goto done;
			}
			error = bridge_timeout(sock, brdg, (char *)argv[0]);
			if (error)
				goto done;
		} else {
			warnx2("unrecognized option: %s\n", argv[0]);
			goto done;
		}
	}

done:
	close(sock);
	return (error);
}

int
bridge_ifsetflag(s, brdg, ifsname, flag)
	int s;
	char *brdg, *ifsname;
	u_int32_t flag;
{
	struct ifbreq req;

	strncpy(req.ifbr_name, brdg, sizeof(req.ifbr_name));
	strncpy(req.ifbr_ifsname, ifsname, sizeof(req.ifbr_ifsname));
	if (ioctl(s, SIOCBRDGGIFFLGS, (caddr_t)&req) < 0) {
		return (1);
	}

	req.ifbr_ifsflags |= flag;

	if (ioctl(s, SIOCBRDGSIFFLGS, (caddr_t)&req) < 0) {
		warnx3("%s: %s", brdg, ifsname);
		return (1);
	}
	return (0);
}

int
bridge_ifclrflag(s, brdg, ifsname, flag)
	int s;
	char *brdg, *ifsname;
	u_int32_t flag;
{
	struct ifbreq req;

	strncpy(req.ifbr_name, brdg, sizeof(req.ifbr_name));
	strncpy(req.ifbr_ifsname, ifsname, sizeof(req.ifbr_ifsname));

	if (ioctl(s, SIOCBRDGGIFFLGS, (caddr_t)&req) < 0) {
		warnx3("%s: %s", brdg, ifsname);
		return (1);
	}

	req.ifbr_ifsflags &= ~flag;

	if (ioctl(s, SIOCBRDGSIFFLGS, (caddr_t)&req) < 0) {
		warnx3("%s: %s", brdg, ifsname);
		return (1);
	}
	return (0);
}

int
bridge_show_all(s)
	int s;
{
	char *inbuf = NULL;
	struct ifconf ifc;
	struct ifreq *ifrp, ifreq;
	int len = 8192, i;

	while (1) {
		ifc.ifc_len = len;
		ifc.ifc_buf = inbuf = realloc(inbuf, len);
		if (inbuf == NULL)
			err(1, "malloc");
		if (ioctl(s, SIOCGIFCONF, &ifc) < 0)
			err(1, "ioctl(SIOCGIFCONF)");
		if (ifc.ifc_len + sizeof(struct ifreq) < len)
			break;
		len *= 2;
	}
	ifrp = ifc.ifc_req;
	ifreq.ifr_name[0] = '\0';
	for (i = 0; i < ifc.ifc_len; ) {
		ifrp = (struct ifreq *)((caddr_t)ifc.ifc_req + i);
		i += sizeof(ifrp->ifr_name) +
		    (ifrp->ifr_addr.sa_len > sizeof(struct sockaddr) ?
		    ifrp->ifr_addr.sa_len : sizeof(struct sockaddr));
		if (ifrp->ifr_addr.sa_family != AF_LINK)
			continue;
		if (!is_bridge(s, ifrp->ifr_name))
			continue;
		bridge_status(s, ifrp->ifr_name);
	}
        free(ifc.ifc_buf);
	return (0);
}

int
bridge_setflag(s, brdg, f)
	int s;
	char *brdg;
	short f;
{
	struct ifreq ifr;

	strncpy(ifr.ifr_name, brdg, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0) {
		warnx2("%s", brdg);
		if (errno == EPERM)
			return (1);
		return (1);
	}

	ifr.ifr_flags |= f;

	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) < 0) {
		warnx2("%s", brdg);
		if (errno == EPERM)
			return (1);
		return (1);
	}

	return (0);
}

int
bridge_clrflag(s, brdg, f)
	int s;
	char *brdg;
	short f;
{
	struct ifreq ifr;

	strncpy(ifr.ifr_name, brdg, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0) {
		warnx2("%s", brdg);
		if (errno == EPERM)
			return (1);
		return (1);
	}

	ifr.ifr_flags &= ~(f);

	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) < 0) {
		warnx2("%s", brdg);
		if (errno == EPERM)
			return (1);
		return (1);
	}

	return (0);
}

int
bridge_flushall(s, brdg)
	int s;
	char *brdg;
{
	struct ifbreq req;

	strncpy(req.ifbr_name, brdg, sizeof(req.ifbr_name));
	req.ifbr_ifsflags = IFBF_FLUSHALL;
	if (ioctl(s, SIOCBRDGFLUSH, &req) < 0) {
		warnx2("%s", brdg);
		return (1);
	}
	return (0);
}

int
bridge_flush(s, brdg)
	int s;
	char *brdg;
{
	struct ifbreq req;

	strncpy(req.ifbr_name, brdg, sizeof(req.ifbr_name));
	req.ifbr_ifsflags = IFBF_FLUSHDYN;
	if (ioctl(s, SIOCBRDGFLUSH, &req) < 0) {
		warnx2("%s", brdg);
		return (1);
	}
	return (0);
}

int
bridge_list(s, brdg, delim)
	int s;
	char *brdg, *delim;
{
	struct ifbreq *reqp;
	struct ifbifconf bifc;
	int i, len = 8192;
	char buf[sizeof(reqp->ifbr_ifsname) + 1], *inbuf = NULL;

	while (1) {
		strncpy(bifc.ifbic_name, brdg, sizeof(bifc.ifbic_name));
		bifc.ifbic_len = len;
		bifc.ifbic_buf = inbuf = realloc(inbuf, len);
		if (inbuf == NULL)
			err(1, "malloc");
		if (ioctl(s, SIOCBRDGIFS, &bifc) < 0)
			err(1, brdg);
		if (bifc.ifbic_len + sizeof(*reqp) < len)
			break;
		len *= 2;
	}
	for (i = 0; i < bifc.ifbic_len / sizeof(*reqp); i++) {
		reqp = bifc.ifbic_req + i;
		strncpy(buf, reqp->ifbr_ifsname, sizeof(buf));
		printf("%s%s ", delim, buf);
		printb("flags", reqp->ifbr_ifsflags, IFBIFBITS);
		printf("\n");
		bridge_rules(s, brdg, buf, delim);
	}
	free(bifc.ifbic_buf);
	return (0);             /* NOTREACHED */
}

int
bridge_add(s, brdg, ifn)
	int s;
	char *brdg, *ifn;
{
	struct ifbreq req;

	strncpy(req.ifbr_name, brdg, sizeof(req.ifbr_name));
	strncpy(req.ifbr_ifsname, ifn, sizeof(req.ifbr_ifsname));
	if (ioctl(s, SIOCBRDGADD, &req) < 0) {
		warnx3("%s: %s", brdg, ifn);
		if (errno == EPERM)
			return (1);
		return (1);
	}
	return (0);
}

int
bridge_delete(s, brdg, ifn)
	int s;
	char *brdg, *ifn;
{
	struct ifbreq req;

	strncpy(req.ifbr_name, brdg, sizeof(req.ifbr_name));
	strncpy(req.ifbr_ifsname, ifn, sizeof(req.ifbr_ifsname));
	if (ioctl(s, SIOCBRDGDEL, &req) < 0) {
		//warnx3("%s: %s", brdg, ifn);
		if (errno == EPERM)
			return (1);
		return (1);
	}
	return (0);
}

int
bridge_timeout(s, brdg, arg)
	int s;
	char *brdg, *arg;
{
	struct ifbcachetoreq ifbct;
	u_int32_t newtime;
	char *endptr;

	newtime = strtoul(arg, &endptr, 0);
	if (arg[0] == '\0' || endptr[0] != '\0') {
		printf("invalid arg for timeout: %s\n", arg);
		return (1);
	}

	strncpy(ifbct.ifbct_name, brdg, sizeof(ifbct.ifbct_name));
	ifbct.ifbct_time = newtime;
	if (ioctl(s, SIOCBRDGSTO, (caddr_t)&ifbct) < 0) {
		warnx2("%s", brdg);
		return (1);
	}
	return (0);
}

int
bridge_maxaddr(s, brdg, arg)
	int s;
	char *brdg, *arg;
{
	struct ifbcachereq ifbc;
	u_int32_t newsize;
	char *endptr;

	newsize = strtoul(arg, &endptr, 0);
	if (arg[0] == '\0' || endptr[0] != '\0') {
		printf("invalid arg for maxaddr: %s\n", arg);
		return (1);
	}

	strncpy(ifbc.ifbc_name, brdg, sizeof(ifbc.ifbc_name));
	ifbc.ifbc_size = newsize;
	if (ioctl(s, SIOCBRDGSCACHE, (caddr_t)&ifbc) < 0) {
		warnx2("%s", brdg);
		return (1);
	}
	return (0);
}

int
bridge_deladdr(s, brdg, addr)
	int s;
	char *brdg, *addr;
{
	struct ifbareq ifba;
	struct ether_addr *ea;

	strncpy(ifba.ifba_name, brdg, sizeof(ifba.ifba_name));
	ea = ether_aton(addr);
	if (ea == NULL) {
		warnx2("Invalid address: %s", addr);
		return (1);
	}
	bcopy(ea, &ifba.ifba_dst, sizeof(struct ether_addr));

	if (ioctl(s, SIOCBRDGDADDR, &ifba) < 0) {
		warnx3("%s: %s", brdg, addr);
		return (1);
	}

	return (0);
}

int
bridge_addaddr(s, brdg, ifname, addr)
	int s;
	char *brdg, *ifname, *addr;
{
	struct ifbareq ifba;
	struct ether_addr *ea;

	strncpy(ifba.ifba_name, brdg, sizeof(ifba.ifba_name));
	strncpy(ifba.ifba_ifsname, ifname, sizeof(ifba.ifba_ifsname));

	ea = ether_aton(addr);
	if (ea == NULL) {
		warnx2("Invalid address: %s", addr);
		return (1);
	}
	bcopy(ea, &ifba.ifba_dst, sizeof(struct ether_addr));
	ifba.ifba_flags = IFBAF_STATIC;

	if (ioctl(s, SIOCBRDGSADDR, &ifba) < 0) {
		warnx3("%s: %s", brdg, addr);
		return (1);
	}

	return (0);
}

int
bridge_addrs(s, brdg, delim)
	int s;
	char *brdg, *delim;
{
	struct ifbaconf ifbac;
	struct ifbareq *ifba;
	char *inbuf = NULL, buf[sizeof(ifba->ifba_ifsname) + 1];
	int i, len = 8192;

	while (1) {
		ifbac.ifbac_len = len;
		ifbac.ifbac_buf = inbuf = realloc(inbuf, len);
		strncpy(ifbac.ifbac_name, brdg, sizeof(ifbac.ifbac_name));
		if (inbuf == NULL)
			err(1, "malloc");
		if (ioctl(s, SIOCBRDGRTS, &ifbac) < 0) {
			if (errno == ENETDOWN)
				return (0);
			err(1, brdg);
		}
		if (ifbac.ifbac_len + sizeof(*ifba) < len)
			break;
		len *= 2;
	}

	for (i = 0; i < ifbac.ifbac_len / sizeof(*ifba); i++) {
		ifba = ifbac.ifbac_req + i;
		strncpy(buf, ifba->ifba_ifsname, sizeof(buf));
		printf("%s%s %s %d ", delim, ether_ntoa(&ifba->ifba_dst),
		    buf, ifba->ifba_age);
		printb("flags", ifba->ifba_flags, IFBAFBITS);
		printf("\n");
	}
        free(ifbac.ifbac_buf);
	return (0);
}

/*
 * Check to make sure 'brdg' is really a bridge interface.
 */
int
is_bridge(s, brdg)
	int s;
	char *brdg;
{
	struct ifreq ifr;
	struct ifbaconf ifbac;

	strncpy(ifr.ifr_name, brdg, sizeof(ifr.ifr_name));

	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		return (0);

	ifbac.ifbac_len = 0;
	strncpy(ifbac.ifbac_name, brdg, sizeof(ifbac.ifbac_name));
	if (ioctl(s, SIOCBRDGRTS, (caddr_t)&ifbac) < 0) {
		if (errno == ENETDOWN)
			return (1);
		return (0);
	}
	if(!strncmp(ifbac.ifbac_name,"ppp0",strlen("ppp0")))
	{
		return(0);
	}
	return (1);
}

int
bridge_status(s, brdg)
	int s;
	char *brdg;
{
	struct ifreq ifr;
	struct ifbcachereq ifbc;
	struct ifbcachetoreq ifbct;
	int err;

	strncpy(ifr.ifr_name, brdg, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0) {
		warnx2("%s", brdg);
		if (errno == EPERM)
			return (1);
		return (1);
	}

	printf("%s: ", brdg);
	printb("flags", ifr.ifr_flags, IFFBITS);
	printf("\n");

	printf("Interfaces:\n");
	err = bridge_list(s, brdg, "  ");
	if (err)
		return (err);

	strncpy(ifbc.ifbc_name, brdg, sizeof(ifbc.ifbc_name));
	if (ioctl(s, SIOCBRDGGCACHE, (caddr_t)&ifbc) < 0) {
		warnx2("%s", brdg);
		return (1);
	}

	strncpy(ifbct.ifbct_name, brdg, sizeof(ifbct.ifbct_name));
	if (ioctl(s, SIOCBRDGGTO, (caddr_t)&ifbct) < 0) {
		warnx2("%s", brdg);
		return (1);
	}

	printf("Addresses (max cache: %d, timeout: %d):\n",
	    ifbc.ifbc_size, ifbct.ifbct_time);

	err = bridge_addrs(s, brdg, "   ");
	return (err);
}


int bridge_get_interfaces(s, brdg,buffer,count)
	int s;
	char *brdg;
	char *buffer;
	int count;
{
	struct ifbreq *reqp;
	struct ifbifconf bifc;
	int i, len = 8192;
	char buf[sizeof(reqp->ifbr_ifsname) + 1], *inbuf = NULL;

	while (1) {
		strncpy(bifc.ifbic_name, brdg, sizeof(bifc.ifbic_name));
		bifc.ifbic_len = len;
		bifc.ifbic_buf = inbuf = realloc(inbuf, len);
		if (inbuf == NULL)
			err(1, "malloc");
		if (ioctl(s, SIOCBRDGIFS, &bifc) < 0)
			err(1, brdg);
		if (bifc.ifbic_len + sizeof(*reqp) < len)
			break;
		len *= 2;
	}
	for (i = 0; i < bifc.ifbic_len / sizeof(*reqp); i++) {
		reqp = bifc.ifbic_req + i;
		if(i > count) {			
 			return -1;
		}		
		strncpy(buf, reqp->ifbr_ifsname, sizeof(buf));		 
		sprintf(buffer+strlen(buffer),"%s ",buf);
	}
	free(bifc.ifbic_buf);
	return (0);             /* NOTREACHED */
}

int
bridge_flush_rule(s, brdg, ifname)
	int s;
	char *brdg, *ifname;
{
	struct ifbrlreq req;

	strncpy(req.ifbr_name, brdg, sizeof(req.ifbr_name));
	strncpy(req.ifbr_ifsname, ifname, sizeof(req.ifbr_ifsname));
	if (ioctl(s, SIOCBRDGFRL, &req) < 0) {
		warnx3("%s: %s", brdg, ifname);
		return (1);
	}
	return (0);
}

int
bridge_rules(s, brdg, ifname, delim)
	int s;
	char *brdg, *ifname;
	char *delim;
{
	char *inbuf = NULL;
	struct ifbrlconf ifc;
	struct ifbrlreq *ifrp, ifreq;
	int len = 8192, i;

	while (1) {
		ifc.ifbrl_len = len;
		ifc.ifbrl_buf = inbuf = realloc(inbuf, len);
		strncpy(ifc.ifbrl_name, brdg, sizeof(ifc.ifbrl_name));
		strncpy(ifc.ifbrl_ifsname, ifname, sizeof(ifc.ifbrl_ifsname));
		if (inbuf == NULL)
			err(1, "malloc");
		if (ioctl(s, SIOCBRDGGRL, &ifc) < 0)
			err(1, "ioctl(SIOCBRDGGRL)");
		if (ifc.ifbrl_len + sizeof(ifreq) < len)
			break;
		len *= 2;
	}
	ifrp = ifc.ifbrl_req;
	for (i = 0; i < ifc.ifbrl_len; i += sizeof(ifreq)) {
		ifrp = (struct ifbrlreq *)((caddr_t)ifc.ifbrl_req + i);
		bridge_showrule(ifrp, delim);
	}
        free(ifc.ifbrl_buf);
	return (0);
}

void
bridge_showrule(r, delim)
	struct ifbrlreq *r;
	char *delim;
{
	if (delim)
		printf("%s    ", delim);
	else
		printf("%s: ", r->ifbr_name);

	if (r->ifbr_action == BRL_ACTION_BLOCK)
		printf("block ");
	else if (r->ifbr_action == BRL_ACTION_PASS)
		printf("pass ");
	else
		printf("[neither block nor pass?]\n");

	if ((r->ifbr_flags & (BRL_FLAG_IN | BRL_FLAG_OUT)) ==
	    (BRL_FLAG_IN | BRL_FLAG_OUT))
		printf("in/out ");
	else if (r->ifbr_flags & BRL_FLAG_IN)
		printf("in ");
	else if (r->ifbr_flags & BRL_FLAG_OUT)
		printf("out ");
	else
		printf("[neither in nor out?]\n");

	printf("on %s", r->ifbr_ifsname);

	if (r->ifbr_flags & BRL_FLAG_SRCVALID)
		printf(" src %s", ether_ntoa(&r->ifbr_src));
	if (r->ifbr_flags & BRL_FLAG_DSTVALID)
		printf(" dst %s", ether_ntoa(&r->ifbr_dst));

	printf("\n");
}

/*
 * Parse a rule definition and send it upwards.
 *
 * Syntax:
 *	{block|pass} {in|out|in/out} on {ifs} [src {mac}] [dst {mac}]
 */
int
bridge_rule(int s, char *brdg, int targc, char **targv, int ln)
{
	char **argv = targv;
	int argc = targc;
	struct ifbrlreq rule;
	struct ether_addr *ea, *dea;

	if (argc == 0) {
		printf( "invalid rule\n");
		return (1);
	}
	rule.ifbr_flags = 0;
	rule.ifbr_action = 0;
	strncpy(rule.ifbr_name, brdg, sizeof(rule.ifbr_name));

	if (strcmp(argv[0], "block") == 0)
		rule.ifbr_action = BRL_ACTION_BLOCK;
	else if (strcmp(argv[0], "pass") == 0)
		rule.ifbr_action = BRL_ACTION_PASS;
	else
		goto bad_rule;
	argc--;	argv++;

	if (argc == 0) {
		bridge_badrule(targc, targv, ln);
		return (1);
	}
	if (strcmp(argv[0], "in") == 0)
		rule.ifbr_flags |= BRL_FLAG_IN;
	else if (strcmp(argv[0], "out") == 0)
		rule.ifbr_flags |= BRL_FLAG_OUT;
	else if (strcmp(argv[0], "in/out") == 0)
		rule.ifbr_flags |= BRL_FLAG_IN | BRL_FLAG_OUT;
	else
		goto bad_rule;
	argc--; argv++;

	if (argc == 0 || strcmp(argv[0], "on"))
		goto bad_rule;
	argc--; argv++;

	if (argc == 0)
		goto bad_rule;
	strncpy(rule.ifbr_ifsname, argv[0], sizeof(rule.ifbr_ifsname));
	argc--; argv++;

	while (argc) {
		if (strcmp(argv[0], "dst") == 0) {
			if (rule.ifbr_flags & BRL_FLAG_DSTVALID)
				goto bad_rule;
			rule.ifbr_flags |= BRL_FLAG_DSTVALID;
			dea = &rule.ifbr_dst;
		}
		else if (strcmp(argv[0], "src") == 0) {
			if (rule.ifbr_flags & BRL_FLAG_SRCVALID)
				goto bad_rule;
			rule.ifbr_flags |= BRL_FLAG_SRCVALID;
			dea = &rule.ifbr_src;
		}
		else
			goto bad_rule;

		argc--; argv++;

		if (argc == 0)
			goto bad_rule;
		ea = ether_aton(argv[0]);
		if (ea == NULL) {
			warnx2("Invalid address: %s", argv[0]);
			return (1);
		}
		bcopy(ea, dea, sizeof(*dea));
		argc--; argv++;
	}

	if (ioctl(s, SIOCBRDGARL, &rule) < 0) {
		warnx2("%s", brdg);
		return (1);
	}
	return (0);

bad_rule:
	bridge_badrule(targc, targv, ln);
	return (1);
}

#define MAXRULEWORDS 8
#ifdef ZAP
int
bridge_rulefile(s, brdg, fname)
	int s;
	char *brdg, *fname;
{
	FILE *f;
	char *str, *argv[MAXRULEWORDS], buf[1024], xbuf[1024];
	int ln = 1, argc = 0, err = 0, xerr;

	f = fopen(fname, "r");
	if (f == NULL) {
		warnx2("%s", fname);
		return (1);
	}

	while (1) {
		fgets(buf, sizeof(buf), f);
		if (feof(f))
			break;
		ln++;
		if (buf[0] == '#')
			continue;

		argc = 0;
		str = strtok(buf, "\n\r ");
		strncpy(xbuf, buf, sizeof(xbuf));
		while (str != NULL) {
			argv[argc++] = str;
			if (argc > MAXRULEWORDS) {
				printf( "invalid rule: %d: %s\n",
				    ln, xbuf);
				break;
			}
			str = strtok(NULL, "\n\r ");
		}

		if (argc > MAXRULEWORDS)
			continue;

		xerr = bridge_rule(s, brdg, argc, argv, ln);
		if (xerr)
			err = xerr;
	}
	fclose(f);
	return (err);
}
#endif
void
bridge_badrule(argc, argv, ln)
	int argc, ln;
	char **argv;
{
	int i;

	printf( "invalid rule: ");
	if (ln != -1)
		printf( "%d: ", ln);
	for (i = 0; i < argc; i++) {
		printf( "%s ", argv[i]);
	}
	printf( "\n");
}

/*
 * Print a value ala the %b format of the kernel's printf
 * (borrowed from ifconfig.c)
 */
/*
void
printb(s, v, bits)
	char *s;
	char *bits;
	unsigned short v;
{
	register int i, any = 0;
	register char c;

	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	bits++;
	if (bits) {
		printf("<");
		while ((i = *bits++)) {
			if (v & (1 << (i-1))) {
				if (any)
					printf(",");
				any = 1;
				for (; (c = *bits) > 32; bits++)
					printf("%c",c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		printf(">");
	}
}

static void 
interface_up(const char *intf)
{
  struct ifreq ifr;
  int s;

  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) {
    perror("socket");
    return;
  }

  strcpy(ifr.ifr_name, intf);
  ifr.ifr_flags = IFF_UP | IFF_BROADCAST | IFF_RUNNING;
  if (ioctl(s, SIOCSIFFLAGS, &ifr)) {
    perror("SIOCSIFFLAGS");
  }
  close(s);
}
*/
#if 0
extern unsigned char active_br_wlan_interface[80];

void set_wlan_bridge(void)
{
	int sock;
	char brdg[] = "bridge0";
	char *token=NULL, *savestr1=NULL;
	unsigned char tmpBuff[80]; 

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sock >= 0);

	if(active_br_wlan_interface[0] != 0x00)
	{
		token=NULL;
		savestr1=NULL;
		sprintf(tmpBuff, "%s", active_br_wlan_interface);
		token = strtok_r(tmpBuff," ", &savestr1);
		do{
			if (token == NULL){
				break;
			}else{
				interface_up(token);
				bridge_add(sock,brdg,token);
#ifdef CYGPKG_NET_BRIDGE_STP_CODE
				bridge_ifsetflag (sock, brdg, token, IFBIF_STP);
#endif
			}
			token = strtok_r(NULL, " ", &savestr1);
		}while(token !=NULL);
	}
	close(sock);
}


//void main_fn(cyg_addrword_t data)
void bridge_main_fn(void)
{
	int sock;
	char brdg[] = "bridge0";
	unsigned int op_mode;
	//struct bootp brdg_bootp_data;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sock >= 0);

	//BRIDGE_IF_NAME is created when kernel init, so we don't need to addbr
#ifndef HAVE_NOETH
#ifdef CYGHWR_NET_DRIVER_ETH0
	interface_up("eth0");
	eth0_up = 1;
	bridge_add(sock,brdg,"eth0");
#ifdef CYGPKG_NET_BRIDGE_STP_CODE
	bridge_ifsetflag (sock, brdg, "eth0", IFBIF_STP);
#endif
#endif

#ifdef CYGHWR_NET_DRIVER_ETH1
	apmib_get(MIB_OP_MODE, (void *)&op_mode);
	if((op_mode == BRIDGE_MODE) || (op_mode == WISP_MODE))
	{
		interface_up("eth1");
		eth1_up = 1;
		bridge_add(sock,brdg,"eth1");
#ifdef CYGPKG_NET_BRIDGE_STP_CODE
		bridge_ifsetflag (sock, brdg, "eth1", IFBIF_STP);
#endif
	}
#endif

#endif

#ifdef RTLPKG_DEVS_ETH_RLTK_819X_WLAN_WLAN0
#ifndef HAVE_APMIB
	//use active_br_wlan_interface for virtual AP, vxd and dual band support
	if(active_br_wlan_interface[0] != 0x00)
	{
		char *token=NULL, *savestr1=NULL;
		unsigned char tmpBuff[80]; 
		
		token=NULL;
		savestr1=NULL;
		sprintf(tmpBuff, "%s", active_br_wlan_interface);
		token = strtok_r(tmpBuff," ", &savestr1);
		do{
			if (token == NULL){
				break;
			}else{
				interface_up(token);
				bridge_add(sock,brdg,token);
#ifdef CYGPKG_NET_BRIDGE_STP_CODE
				bridge_ifsetflag (sock, brdg, token, IFBIF_STP);
#endif
			}
			token = strtok_r(NULL, " ", &savestr1);
		}while(token !=NULL);
	}
#else
	set_wlan_bridge();
#endif

#endif

	bridge_status(sock,brdg);
	bridge_setflag(sock,brdg, IFF_UP);

	/*
	build_bootp_record(&brdg_bootp_data,
                           "eth0",
                           "192.168.1.254",
                           "255.255.255.0",
                           "192.168.1.255",
                           "192.168.1.101",
                           "192.168.1.55");
	//show_bootp("eth0", &brdg_bootp_data);
  	if (!init_net("eth0", &brdg_bootp_data)) {
            printf("Network initialization failed for %s\n", brdg);
	}*/

	/*
	while(1) {
		cyg_kmem_print_stats();
		printf("------------------------------------------------\n");
		bridge_show_all(sock);
		printf("------------------------------------------------\n");
		cyg_thread_delay(500);
	}
	*/
	close(sock);
}

void reset_wlan_bridge(void)
{
	int sock;
	char brdg[] = "bridge0";
	char *token=NULL, *savestr1=NULL;
	unsigned char tmpBuff[80]; 

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	assert(sock >= 0);

	//use active_br_wlan_interface for virtual AP, vxd and dual band support

	if(active_br_wlan_interface[0] != 0x00)
	{
		
		token=NULL;
		savestr1=NULL;
		sprintf(tmpBuff, "%s", active_br_wlan_interface);
		token = strtok_r(tmpBuff," ", &savestr1);
		do{
			if (token == NULL){
				break;
			}else{
				printf("====%s(%d)\n", __FUNCTION__, __LINE__);
				bridge_delete(sock, brdg, tmpBuff);
			}
			token = strtok_r(NULL, " ", &savestr1);
		}while(token !=NULL);
	}
	
	set_wlan_bridge();
	close(sock);
}
#endif

#endif //CYGINT_NET_BRIDGE_HANDLER
