#include "ifaddrs.h"

/* Create a linked list of `struct ifaddrs' structures, one for each
   network interface on the host machine.  If successful, store the
   list in *IFAP and return 0.  On errors, return -1 and set `errno'.

   The storage returned in *IFAP is allocated dynamically and can
   only be properly freed by passing it to `freeifaddrs'.  */
int getifaddrs (struct ifaddrs **__ifap) {
  *__ifap = 0; // nullptr
  return 0;
}

/* Reclaim the storage allocated by a previous `getifaddrs' call.  */
void freeifaddrs (struct ifaddrs *__ifa) {

}