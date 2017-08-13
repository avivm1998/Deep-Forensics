/*
* Includes
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/sched.h>

#include "lwip/init.h"
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
#include "tcp_raw.h"

#define SUCCESS 0
#define DEBUG 1
#define EMPTY_MESSAGE "NO_DATA"
#define SIZE 1024

/*
* Prototypes
*/

int init_module(void);
void cleanup_module(void);
void init_lwip_server(void);
void thread_function(void* args);
int copy_data_from_memory(int start_address, int length, char* data, int buffer_length);
int split(char*** parts, const char* str, const char* delimiter);