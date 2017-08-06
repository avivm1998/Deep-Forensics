/*
  -change file name 
  -integaete with client
  -itergarte with kernel 


*/

#include <string.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <unistd.h>

#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "tcpecho_raw.h"

#define PORT 1202
#define NETLINK_USER 31
#define MAX_PAYLOAD 256

#if LWIP_TCP

static struct tcp_pcb *tcpecho_raw_pcb;

/* Initializing the netlink socket to connect to the kernel */
static void init_netlink_socket(int* nl_fd, struct sockaddr_nl* src_addr, struct sockaddr_nl* dest_addr, struct nlmsghdr** nlh) {

	int optval = 1;

	if ((*nl_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER)) < 0) {
		perror("netlink socket");
		exit(1);
	}

	/*memset(src_addr, 0, sizeof(*src_addr));*/
	src_addr->nl_family = AF_NETLINK;
	src_addr->nl_pid = getpid(); /* self pid */ 
	src_addr->nl_groups = 0; /* not in mcast group */

	if(bind(*nl_fd, (struct sockaddr *)src_addr, sizeof(*src_addr)) < 0) {
		perror("netlink bind");
		exit(1);
	}

	/*memset(dest_addr, 0, sizeof(*dest_addr));*/
	dest_addr->nl_family = AF_NETLINK;
	dest_addr->nl_pid = 0; /* Linux kernel */
	dest_addr->nl_groups = 0; /* unicast */

  setsockopt(*nl_fd, SOL_SOCKET, NETLINK_NO_ENOBUFS, &optval, sizeof(optval));

	*nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
  memset(*nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
  (*nlh)->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
  (*nlh)->nlmsg_pid = getpid();
  (*nlh)->nlmsg_flags = 0;
}

static char* send_netlink_request(int address, int length) {
  char buffer[MAX_PAYLOAD] = { 0 };

  int nl_fd = 0;
  struct sockaddr_nl src_addr = { 0 };
  struct sockaddr_nl dest_addr = { 0 };
  struct nlmsghdr *nlh = NULL;
  struct iovec iov = { 0 };
  struct msghdr msg = { 0 };

  init_netlink_socket(&nl_fd, &src_addr, &dest_addr, &nlh);

  sprintf(buffer,"%08x,%08x", address, length);
  strncpy(NLMSG_DATA(nlh), buffer, strlen(buffer) + 1);

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;
  msg.msg_name = (void *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  if(sendmsg(nl_fd, &msg, 0) < 0) {
    perror("sendmsg");
    return NULL;
  }

  if(recvmsg(nl_fd, &msg, 0) < 0) {
    perror("recvmsg");
    return NULL;
  }

  close(nl_fd);

  return (char*)NLMSG_DATA(nlh);
}

enum tcpecho_raw_states
{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

struct tcpecho_raw_state
{
  u8_t state;
  u8_t retries;
  struct tcp_pcb *pcb;
  /* pbuf (chain) to recycle */
  struct pbuf *p;
};

static void
tcpecho_raw_free(struct tcpecho_raw_state *es)
{
  if (es != NULL) {
    if (es->p) {
      /* free the buffer chain if present */
      pbuf_free(es->p);
    }

    mem_free(es);
  }
}

static void do_action(struct tcpecho_raw_state *es)
{
  int* data;
  char* res;

  data = (int*)es->p->payload;
  printf("0x%0x,%d\n",data[0],data[1]);

  /*get data*/
  /*to be replaced with transfer to kernel*/
  res = send_netlink_request(data[0],data[1]);  

  es->p->len = data[1];
  memcpy(es->p->payload,res,data[1]);
  
}

static void
tcpecho_raw_close(struct tcp_pcb *tpcb, struct tcpecho_raw_state *es)
{
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);

  tcpecho_raw_free(es);

  tcp_close(tpcb);
}

static void
tcpecho_raw_send(struct tcp_pcb *tpcb, struct tcpecho_raw_state *es)
{
  struct pbuf *ptr;
  err_t wr_err = ERR_OK;

  while ((wr_err == ERR_OK) &&(es->p != NULL) &&(es->p->len <= tcp_sndbuf(tpcb))) {
    ptr = es->p;

    /* enqueue data for transmission */
    wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
    if (wr_err == ERR_OK) {
      u16_t plen;

      plen = ptr->len;
      /* continue with next pbuf in chain (if any) */
      es->p = ptr->next;
      if(es->p != NULL) {
        /* new reference! */
        pbuf_ref(es->p);
      }
      /* chop first pbuf from chain */
      pbuf_free(ptr);
      /* we can read more data now */
      tcp_recved(tpcb, plen);
    } else if(wr_err == ERR_MEM) {
      /* we are low on memory, try later / harder, defer to poll */
      es->p = ptr;
    } else {
      /* other problem ?? */
    }
  }
}

static void
tcpecho_raw_error(void *arg, err_t err)
{
  struct tcpecho_raw_state *es;

  LWIP_UNUSED_ARG(err);

  es = (struct tcpecho_raw_state *)arg;

  tcpecho_raw_free(es);
}

static err_t
tcpecho_raw_poll(void *arg, struct tcp_pcb *tpcb)
{
  err_t ret_err;
  struct tcpecho_raw_state *es;

  es = (struct tcpecho_raw_state *)arg;
  if (es != NULL) {
    if (es->p != NULL) {
      /* there is a remaining pbuf (chain)  */
      tcpecho_raw_send(tpcb, es);
    } else {
      /* no remaining pbuf (chain)  */
      if(es->state == ES_CLOSING) {
        tcpecho_raw_close(tpcb, es);
      }
    }
    ret_err = ERR_OK;
  } else {
    /* nothing to be done */
    tcp_abort(tpcb);
    ret_err = ERR_ABRT;
  }
  return ret_err;
}

static err_t
tcpecho_raw_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct tcpecho_raw_state *es;

  LWIP_UNUSED_ARG(len);

  es = (struct tcpecho_raw_state *)arg;
  es->retries = 0;

  if(es->p != NULL) {
    /* still got pbufs to send */
    tcp_sent(tpcb, tcpecho_raw_sent);
    tcpecho_raw_send(tpcb, es);
  } else {
    /* no more pbufs to send */
    if(es->state == ES_CLOSING) {
      tcpecho_raw_close(tpcb, es);
    }
  }
  return ERR_OK;
}

static err_t
tcpecho_raw_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  struct tcpecho_raw_state *es;
  err_t ret_err;

  LWIP_ASSERT("arg != NULL",arg != NULL);
  es = (struct tcpecho_raw_state *)arg;
  if (p == NULL) {
    /* remote host closed connection */
    es->state = ES_CLOSING;
    if(es->p == NULL) {
      /* we're done sending, close it */
      tcpecho_raw_close(tpcb, es);
    } else {
      /* we're not done yet */
      tcpecho_raw_send(tpcb, es);
    }
    ret_err = ERR_OK;
  } else if(err != ERR_OK) {
    /* cleanup, for unknown reason */
    if (p != NULL) {
      pbuf_free(p);
    }
    ret_err = err;
  }
  else if(es->state == ES_ACCEPTED) {
    /* first data chunk in p->payload */
    es->state = ES_RECEIVED;
    /* store reference to incoming pbuf (chain) */
    es->p = p;
    do_action(es);
    tcpecho_raw_send(tpcb, es);
    ret_err = ERR_OK;
  } else if (es->state == ES_RECEIVED) {
    /* read some more data */
    if(es->p == NULL) {
      es->p = p;
      do_action(es);
      tcpecho_raw_send(tpcb, es);

    } else {
      struct pbuf *ptr;

      /* chain pbufs to the end of what we recv'ed previously  */
      ptr = es->p;
      pbuf_cat(ptr,p);
    }
    ret_err = ERR_OK;
  } else {
    /* unkown es->state, trash data  */
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  return ret_err;
}

static err_t
tcpecho_raw_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  err_t ret_err;
  struct tcpecho_raw_state *es;

  LWIP_UNUSED_ARG(arg);
  if ((err != ERR_OK) || (newpcb == NULL)) {
    return ERR_VAL;
  }

  /* Unless this pcb should have NORMAL priority, set its priority now.
     When running out of pcbs, low priority pcbs can be aborted to create
     new pcbs of higher priority. */
  tcp_setprio(newpcb, TCP_PRIO_MIN);

  es = (struct tcpecho_raw_state *)mem_malloc(sizeof(struct tcpecho_raw_state));
  if (es != NULL) {
    es->state = ES_ACCEPTED;
    es->pcb = newpcb;
    es->retries = 0;
    es->p = NULL;
    /* pass newly allocated es to our callbacks */
    tcp_arg(newpcb, es);
    tcp_recv(newpcb, tcpecho_raw_recv);
    tcp_err(newpcb, tcpecho_raw_error);
    tcp_poll(newpcb, tcpecho_raw_poll, 0);
    tcp_sent(newpcb, tcpecho_raw_sent);
    ret_err = ERR_OK;
  } else {
    ret_err = ERR_MEM;
  }
  return ret_err;
}

void
tcpecho_raw_init(void)
{
  tcpecho_raw_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
  if (tcpecho_raw_pcb != NULL) {
    err_t err;

    err = tcp_bind(tcpecho_raw_pcb, IP_ANY_TYPE, 1202);
    if (err == ERR_OK) {
      tcpecho_raw_pcb = tcp_listen(tcpecho_raw_pcb);
      tcp_accept(tcpecho_raw_pcb, tcpecho_raw_accept);
    } else {
      /* abort? output diagnostic? */
    }
  } else {
    /* abort? output diagnostic? */
  }
}

#endif /* LWIP_TCP */
