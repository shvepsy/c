#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
//#include <netinet/eth.h>
//

#include <linux/sockios.h>
#include <linux/ioctl.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#define DEF_IF_NAME "eth0"
#define PROMISC_ON 1
#define PROMISC_OFF 0
#define HEADERS_LEN (sizeof(struct tcphdr) + sizeof(struct iphdr) + sizeof(struct tcphdr))

struct ifparam {
  uint32_t ip;
  uint32_t mask;
  int mtu;
  int index;
} ifp;

int getifconf( uint8_t *ifn, struct ifparam *ifp, int mode )
{
  int fd;
  struct sockaddr_in s;
  struct ifreq ifr;
  memset((void *)&ifr, 0, sizeof(struct ifreq));
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0) ) < 0 ) return (-1);
  sprintf(ifr.ifr_name, "%s", ifn);
  if (ioctl(fd, SIOCGIFADDR, &ifr) < 0 ) {
    perror("SIOCGIFADDR");
    return (-1);
  }
  memset((void *)&s, 0, sizeof(struct sockaddr_in));
  memcpy((void *)&s, (void *)&ifr.ifr_addr, sizeof(struct sockaddr));
  memcpy((void *)&ifp->ip, (void *)&s.sin_addr.s_addr, sizeof(__u32)); //uint32_t ? u_long

  if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0 ) {
    perror("SIOCGIFNETMASK");
    return (-1);
  }
  memset((void *)&s, 0, sizeof(struct sockaddr_in));
  memcpy((void *)&s, (void *)&ifr.ifr_netmask, sizeof(struct sockaddr));
  memcpy((void *)&ifp->mask, (void *)&s.sin_addr.s_addr, sizeof(__u32));

  if (ioctl(fd, SIOCGIFMTU, &ifr) < 0 ) {
    perror("SIOCGIFMTU");
    return (-1);
  }
  //strcpy(ifp->mtu, ifr.ifr_mtu);
  ifp->mtu = ifr.ifr_mtu;
  if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0 ) {
    perror("SIOCGIFINDEX");
    return (-1);
  }
  //strcpy(ifp->index, ifr.ifr_ifindex);
  ifp->index = ifr.ifr_ifindex;
  if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0 ) {
    perror("SIOCGIFFLAGS");
    return (-1);
  }

  if (mode) ifr.ifr_flags |= IFF_PROMISC;
  else ifr.ifr_flags &= ~(IFF_PROMISC);

  if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0 ) {
    perror("SIOCSIFFLAGS");
    close(fd);
    return (-1);
  }
  //ioctl( *ifn, )
  return 0;
}

int prosmic_off() {
    if (getifconf(DEF_IF_NAME, &ifp, PROMISC_OFF) < 0) {
      perror("getifconf");
      exit(-1);
    };
    return;
}

int getsock( int ifIndex )
{
  int sd;
  struct sockaddr_ll s_ll;
  if ((sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0 ) {
    perror("socket");
    return (-1);
  }
  memset((void *)&s_ll, 0, sizeof(struct sockaddr_ll));
  s_ll.sll_family = PF_PACKET;
  s_ll.sll_protocol = htons(ETH_P_ALL);
  s_ll.sll_ifindex = ifIndex;

  if (bind(sd, (struct sockaddr *)&s_ll, sizeof(struct sockaddr_ll)) < 0) {
    close(sd);
    perror("bind");
    return (-1);
  }
  return sd;
}

/* arcopt + ... */
int main( int argc, char *argv[] )
{
  int rec = 0, sock_if;
  uint8_t buf[ETH_FRAME_LEN];
  uint8_t data[(ETH_FRAME_LEN-HEADERS_LEN)];
  uint32_t num;
  struct ethhdr ehdr;
  struct iphdr ihdr;
  struct tcphdr thdr;
  char *ifname;
  if (argc > 1)
    //strcpy(ifname, argc[1]);
    ifname = argv[1];
  else
    //strcpy(ifname, DEF_IF_NAME);
    ifname = DEF_IF_NAME;
  if ( getifconf(ifname, &ifp , PROMISC_ON) < 0 ) {
    perror("getifconf");
    return (-1);
  }

  printf("\nCurrent conf: %s/", inet_ntoa(*(struct in_addr *)&ifp.ip));
  printf("%s\nMTU: %d\n", inet_ntoa(*(struct in_addr *)&ifp.mask), ifp.mtu);
  printf("IfIndex: %d\n", ifp.index);
  //printf("\nCurrent conf:%s %s\nMTU:%d\nIfIndex: %d", inet_ntoa(*(struct in_addr *)&ifp.ip), inet_ntoa(*(struct in_addr *)&ifp.mask), ifp.mtu, ifp.index);

  if ((sock_if = getsock(ifp.index)) < 0) {
    perror("getsock");
    return (-1);
  }

  for (;;) {
    //memset(buf, 0, sizeof(ETH_FRAME_LEN));
    memset(buf, 0, sizeof(ETH_FRAME_LEN));
    memset(data, 0, sizeof(ETH_FRAME_LEN-HEADERS_LEN));
    //int *pbuf = buf;
    rec = recvfrom(sock_if, (char *)&buf, ifp.mtu + 18, 0, NULL, NULL);
    if (rec < 0) {
      perror("recvfrom");
      return (-1);
    }
    memcpy((void *)&ehdr, buf, sizeof(struct ethhdr));
    memcpy((void *)&ihdr, buf + sizeof(struct iphdr), sizeof(struct iphdr));
    memcpy((void *)&thdr, buf + sizeof(struct tcphdr) + sizeof(struct iphdr), sizeof(struct tcphdr));
    memcpy((void *)&data, buf + HEADERS_LEN, (ETH_FRAME_LEN-HEADERS_LEN) );
    /* dig output */
    printf("%d %.2x:%.2x:%.2x:%.2x:%.2x:%.2x ->", num, ehdr.h_source[0], ehdr.h_source[1], ehdr.h_source[2], ehdr.h_source[3], ehdr.h_source[4], ehdr.h_source[5]);
    printf(" %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\t", ehdr.h_dest[0], ehdr.h_dest[1], ehdr.h_dest[2], ehdr.h_dest[3], ehdr.h_dest[4], ehdr.h_dest[5]);
    printf("%15s:%-5.d ->  %s:%d\t%d\n\n%s\n\n", inet_ntoa(*(struct in_addr *)&ihdr.saddr), ntohs(thdr.source), inet_ntoa(*(struct in_addr *)&ihdr.daddr), ntohs(thdr.dest), data);
    // printf("%s ")
    num++;
  }
}
