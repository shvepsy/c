#define PROMISC_MODE_ON 1
#define PROMISC_MODE_OFF 0

#include <linux/types.h>

struct ifparam {
  u_int32_t ip;   // IP address
  u_int32_t mask; // subnet mask
  int mtu;    // MTU size
  int index;  // iface index
} ifp;
