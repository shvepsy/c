#define PROMISC_MODE_ON 1
#define PROMISC_MODE_OFF 0

#include <linux/types.h>

struct ifparam {
  __u32 ip;   // IP address
  __u32 mask; // subnet mask
  int mtu;    // MTU size
  int index;  // iface index
} ifp;
