#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/ioctl.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/sockios.h>
#include "iface.h"

/*
  Write to nbook ntoa, l234 struct, recvfrom
*/

int getifconf( uint8_t *intf, struct ifparam *ifp, int mode )
{
  int fd;
  struct sockaddr_in s;
  struct ifreq ifr;     /* interface name from if.h */

  memset((void *)&ifr, 0, sizeof(struct ifreq));
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return (-1);
  sprintf(ifr.ifr_name, "%s", intf);

  if (!mode) goto setmode;

    /* Getting IP address of interface */
  if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
    perror("ioctl SIOCGIFADDR");
    return (-1);
  }
  memset((void *)&s, 0, sizeof(struct sockaddr_in));
  memcpy((void *)&s, (void *)&ifr.ifr_addr, sizeof(struct sockaddr));
  memcpy((void *)&ifp->ip, (void *)&s.sin_addr.s_addr, sizeof(__u32));

    /* Getting netmask */
  if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0) {
    perror("ioctl SIOCGIFNETMASK");
    return (-1);
  }
  memset((void *)&s, 0, sizeof(struct sockaddr_in));
  memcpy((void *)&s, (void *)&ifr.ifr_netmask, sizeof(struct sockaddr));
  memcpy((void *)&ifp->mask, (void *)&s.sin_addr.s_addr, sizeof(u_long));

    /* Getting MTU */
  if(ioctl(fd, SIOCGIFMTU, &ifr) < 0) {
 	  perror("ioctl SIOCGIFMTU");
 	  return (-1);
  }
  ifp->mtu = ifr.ifr_mtu;

  /* Getting iface index */
  if(ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
    perror("ioctl SIOCGIFINDEX");
    return (-1);
  }
  ifp->index = ifr.ifr_ifindex;

    /* Getting flags */
  setmode:
  if(ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
    perror("ioctl SIOCGIFFLAGS");
    close(fd);
    return (-1);
  }

  if(mode) ifr.ifr_flags |= IFF_PROMISC ;
  else ifr.ifr_flags &= ~(IFF_PROMISC);

  /* Flag set */
  if(ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
    perror("ioctl SIOCSIFFLAGS");
    close(fd);
    return(-1);
  }

  return 0;
}

int getsock_recv(int index)
{
  int sd;
  struct sockaddr_ll s_ll; /* low-level structure for socket */

  if(sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)) < 0 ) return (-1);
  /* create l2 socket */

  memset((void *)&s_ll, 0, sizeof(struct sockaddr_ll));

  s_ll.sll_family = AF_PACKET;
  s_ll.sll_protocol = htons(ETH_P_ALL);
  s_ll.sll_ifindex = ifp.index;          /* or just index? */


  if(bind(sd, (struct sockaddr *)&s_ll, sizeof(struct sockaddr_ll)) < 0 ) {
    //perror("bind");
    close(sd);
    return (-1);
  }
  return sd;
}


uint8_t buf[ETH_FRAME_LEN];

void mode_off()
{
  if(getifconf("eth0", &ifp, PROMISC_MODE_OFF) < 0) {
    perror("getifconf");
    exit(-1);
  }
  return;
}

int main() {
  int32_t num;
  int sock_if, rec = 0, ihl = 0;
  struct ethhdr eth;
  struct tcphdr tcp;                    /* l2 l3 l4 headers */
  struct iphdr ip;
  static struct sigaction act;          /* from <signal.h> to override SIGINT (remove promisc flag) */

  if(getifconf("eth0", &ifp, PROMISC_MODE_ON) < 0 ) {
    perror("getifconf");
    return (-1);
  }
  //char *ipa = (char *);
  //struct in_addr ipa =;
  printf("\n===== Current configuration ===== \nIP:\t%s\n",
    inet_ntoa(*(struct in_addr *)&ifp.ip));
  printf("MASK:\t%s\n", inet_ntoa(*(struct in_addr *)&ifp.mask));
  printf("MTU:\t%d\n", ifp.mtu);
  printf("IIndex:\t%d\n\n", ifp.index);

  if(sock_if = getsock_recv(ifp.index) < 0) {
    perror("getsock_recv");
    return (-1);
  }

  act.sa_handler = mode_off;             /* Overriding func */
  sigfillset(&(act.sa_mask));           /* Get current signal mask */
  sigaction(SIGINT, &act, NULL);       /* Override SIGINT */

  for(;;) {
    memset(buf, 0, ETH_FRAME_LEN);

    rec = recvfrom(sock_if, (char *)buf, ifp.mtu + 18, 0, NULL, NULL );
    if(rec < 0 || rec > ETH_FRAME_LEN) {
      perror("recvfrom");
      return (-1);
    }
    memcpy(&eth, buf, ETH_HLEN);
    memcpy(&ip, buf + ETH_HLEN, sizeof(struct iphdr));
    memcpy(&tcp, buf + ETH_HLEN + sizeof(struct iphdr), sizeof(struct tcphdr));

      /* Packet information */
    printf("\n%u\n",num++);

    printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\t->\t",
    eth.h_source[0], eth.h_source[1], eth.h_source[2],
    eth.h_source[3], eth.h_source[4], eth.h_source[5]);

    printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
    eth.h_dest[0], eth.h_dest[1], eth.h_dest[2],
    eth.h_dest[3], eth.h_dest[4], eth.h_dest[5]);

    /* Byte, 5-15 value from 4bit field * 4byte per one */
    printf("IP header length - %d, ", (ip.ihl * 4));
    printf("IP total length - %d, ", ntohs(ip.tot_len));
    /*struct in_addr saddr;
    struct in_addr daddr;
    saddr = *(struct in_addr *)&ip.saddr;
    daddr = *(struct in_addr *)&ip.daddr; */
    if(ip.protocol == IPPROTO_TCP) {
      printf("TCP, %-15s:%d -> %-15s:%d",
      inet_ntoa(*(struct in_addr *)&ip.saddr), ntohs(tcp.source),
      inet_ntoa(*(struct in_addr *)&ip.daddr), ntohs(tcp.dest) );
    }
  }
  return 0;
}
