
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "lwip/init.h"

#include "lwip/debug.h"

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"

#include "lwip/stats.h"

#include "lwip/ip.h"
#include "lwip/ip4_frag.h"
#include "lwip/tcp.h"
#include "netif/tapif.h"
#include "netif/etharp.h"

#include "tcpecho_raw.h"

/* (manual) host IP configuration */
static ip4_addr_t ipaddr, netmask, gw;

/* nonstatic debug cmd option, exported in lwipopts.h */
unsigned char debug_flags;
#define NUM_OPTS ((sizeof(longopts) / sizeof(struct option)) - 1)


int main(int argc, char **argv)
{
	struct netif netif;

	LWIP_UNUSED_ARG(argc);
	LWIP_UNUSED_ARG(argv);

  	/* startup defaults (may be overridden by one or more opts) */
	IP4_ADDR(&gw, 192,168,0,1);
	IP4_ADDR(&ipaddr, 192,168,0,2);
	IP4_ADDR(&netmask, 255,255,255,0);

	/* use debug flags defined by debug.h */
	debug_flags = LWIP_DBG_OFF;

	lwip_init();

	printf("TCP/IP initialized.\n");

	netif_add(&netif, &ipaddr, &netmask, &gw, NULL, tapif_init, ethernet_input);
	netif_set_default(&netif);
	netif_set_up(&netif);

	tcpecho_raw_init();

	printf("Applications started.\n");

	while(1) {
  		/* poll netif, pass packet to lwIP */
  		tapif_select(&netif);

  		sys_check_timeouts();
	}
	return 0;
}
