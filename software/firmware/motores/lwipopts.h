#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Necessário para pico_cyw43_arch_lwip_threadsafe_background com socket API
#define NO_SYS                      0
#define LWIP_SOCKET                 1
#define LWIP_COMPAT_SOCKETS         1
#define LWIP_POSIX_SOCKETS_IO_NAMES 1

// Rede
#define LWIP_IPV4                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DNS                    1
#define LWIP_DHCP                   1
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1

// Memória
#define MEM_LIBC_MALLOC             0
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4000
#define PBUF_POOL_SIZE              24
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10

// TCP
#define TCP_MSS                     1460
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_TCP_KEEPALIVE          1

// Callbacks e hostname
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETIF_TX_SINGLE_PBUF   1
#define DHCP_DOES_ARP_CHECK         0
#define LWIP_DHCP_DOES_ACD_CHECK    0

// Threads (usadas pelo sys layer do Pico SDK)
#define TCPIP_THREAD_STACKSIZE      2048
#define TCPIP_MBOX_SIZE             16
#define DEFAULT_UDP_RECVMBOX_SIZE   16
#define DEFAULT_TCP_RECVMBOX_SIZE   16
#define DEFAULT_ACCEPTMBOX_SIZE     16
#define DEFAULT_THREAD_STACKSIZE    1024

// Checksum via hardware
#define LWIP_CHKSUM_ALGORITHM       3

#define LWIP_STATS                  0
#define LWIP_DEBUG                  0

#endif /* _LWIPOPTS_H */
