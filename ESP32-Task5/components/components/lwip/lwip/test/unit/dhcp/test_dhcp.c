#include "test_dhcp.h"

#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"

struct netif net_test;

static const u8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static const u8_t magic_cookie[] = { 0x63, 0x82, 0x53, 0x63 };

static u8_t dhcp_offer[] = {
    0x00, 0x23, 0xc1, 0xde, 0xd0, 0x0d, /* To unit */
    0x00, 0x0F, 0xEE, 0x30, 0xAB, 0x22, /* From Remote host */
    0x08, 0x00, /* Protocol: IP */
    0x45, 0x10, 0x01, 0x48, 0x00, 0x00, 0x00, 0x00, 0x80, 0x11, 0x36, 0xcc, 0xc3, 0xaa, 0xbd, 0xab, 0xc3, 0xaa, 0xbd, 0xc8, /* IP header */
    0x00, 0x43, 0x00, 0x44, 0x01, 0x34, 0x00, 0x00, /* UDP header */

    0x02, /* Type == Boot reply */
    0x01, 0x06, /* Hw Ethernet, 6 bytes addrlen */
    0x00, /* 0 hops */
    0xAA, 0xAA, 0xAA, 0xAA, /* Transaction id, will be overwritten */
    0x00, 0x00, /* 0 seconds elapsed */
    0x00, 0x00, /* Flags (unicast) */
    0x00, 0x00, 0x00, 0x00, /* Client ip */
    0xc3, 0xaa, 0xbd, 0xc8, /* Your IP */
    0xc3, 0xaa, 0xbd, 0xab, /* DHCP server ip */
    0x00, 0x00, 0x00, 0x00, /* relay agent */
    0x00, 0x23, 0xc1, 0xde, 0xd0, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* MAC addr + padding */

    /* Empty server name and boot file name */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x63, 0x82, 0x53, 0x63, /* Magic cookie */
    0x35, 0x01, 0x02, /* Message type: Offer */
    0x36, 0x04, 0xc3, 0xaa, 0xbd, 0xab, /* Server identifier (IP) */
    0x33, 0x04, 0x00, 0x00, 0x00, 0x78, /* Lease time 2 minutes */
    0x03, 0x04, 0xc3, 0xaa, 0xbd, 0xab, /* Router IP */
    0x01, 0x04, 0xff, 0xff, 0xff, 0x00, /* Subnet mask */
    0xff, /* End option */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Padding */
};

static u8_t dhcp_ack[] = {
    0x00, 0x23, 0xc1, 0xde, 0xd0, 0x0d, /* To unit */
    0x00, 0x0f, 0xEE, 0x30, 0xAB, 0x22, /* From remote host */
    0x08, 0x00, /* Proto IP */
    0x45, 0x10, 0x01, 0x48, 0x00, 0x00, 0x00, 0x00, 0x80, 0x11, 0x36, 0xcc, 0xc3, 0xaa, 0xbd, 0xab, 0xc3, 0xaa, 0xbd, 0xc8, /* IP header */
    0x00, 0x43, 0x00, 0x44, 0x01, 0x34, 0x00, 0x00, /* UDP header */
    0x02, /* Bootp reply */
    0x01, 0x06, /* Hw type Eth, len 6 */
    0x00, /* 0 hops */
    0xAA, 0xAA, 0xAA, 0xAA,
    0x00, 0x00, /* 0 seconds elapsed */
    0x00, 0x00, /* Flags (unicast) */
    0x00, 0x00, 0x00, 0x00, /* Client IP */
    0xc3, 0xaa, 0xbd, 0xc8, /* Your IP */
    0xc3, 0xaa, 0xbd, 0xab, /* DHCP server IP */
    0x00, 0x00, 0x00, 0x00, /* Relay agent */
    0x00, 0x23, 0xc1, 0xde, 0xd0, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Macaddr + padding */

    /* Empty server name and boot file name */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x63, 0x82, 0x53, 0x63, /* Magic cookie */
    0x35, 0x01, 0x05, /* Dhcp message type ack */
    0x36, 0x04, 0xc3, 0xaa, 0xbd, 0xab, /* DHCP server identifier */
    0x33, 0x04, 0x00, 0x00, 0x00, 0x78, /* Lease time 2 minutes */
    0x03, 0x04, 0xc3, 0xaa, 0xbd, 0xab, /* Router IP */
    0x01, 0x04, 0xff, 0xff, 0xff, 0x00, /* Netmask */
    0xff, /* End marker */

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Padding */
};

static const u8_t arpreply[] = {
    0x00, 0x23, 0xC1, 0xDE, 0xD0, 0x0D, /* dst mac */
    0x00, 0x32, 0x44, 0x20, 0x01, 0x02, /* src mac */
    0x08, 0x06, /* proto arp */
    0x00, 0x01, /* hw eth */
    0x08, 0x00, /* proto ip */
    0x06, /* hw addr len 6 */
    0x04, /* proto addr len 4 */
    0x00, 0x02, /* arp reply */
    0x00, 0x32, 0x44, 0x20, 0x01, 0x02, /* sender mac */
    0xc3, 0xaa, 0xbd, 0xc8, /* sender ip */
    0x00, 0x23, 0xC1, 0xDE, 0xD0, 0x0D, /* target mac */
    0x00, 0x00, 0x00, 0x00, /* target ip */
};

static int txpacket;
static enum tcase {
  TEST_LWIP_DHCP,
  TEST_LWIP_DHCP_NAK,
  TEST_LWIP_DHCP_RELAY,
  TEST_LWIP_DHCP_NAK_NO_ENDMARKER,
  TEST_LWIP_DHCP_INVALID_OVERLOAD,
  TEST_NONE
} tcase;

static int debug = 0;
static void setdebug(int a) {debug = a;}

static int tick = 0;
static void tick_lwip(void)
{
  tick++;
  if (tick % 5 == 0) {
    dhcp_fine_tmr();
  }
  #if ESP_DHCP
  if (tick % 10 == 0) {
#else
  if (tick % 600 == 0) {
#endif
    dhcp_coarse_tmr();
  }
}

static void send_pkt(struct netif *netif, const u8_t *data, size_t len)
{
  struct pbuf *p, *q;
  LWIP_ASSERT("pkt too big", len <= 0xFFFF);
  p = pbuf_alloc(PBUF_RAW, (u16_t)len, PBUF_POOL);

  if (debug) {
    /* Dump data */
    u32_t i;
    printf("RX data (len %d)", p->tot_len);
    for (i = 0; i < len; i++) {
      printf(" %02X", data[i]);
    }
    printf("\n");
  }

  fail_unless(p != NULL);
  for(q = p; q != NULL; q = q->next) {
    memcpy(q->payload, data, q->len);
    data += q->len;
  }
  netif->input(p, netif);
}

static err_t lwip_tx_func(struct netif *netif, struct pbuf *p);

static err_t testif_init(struct netif *netif)
{
  netif->name[0] = 'c';
  netif->name[1] = 'h';
  netif->output = etharp_output;
  netif->linkoutput = lwip_tx_func;
  netif->mtu = 1500;
  netif->hwaddr_len = 6;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

  netif->hwaddr[0] = 0x00;
  netif->hwaddr[1] = 0x23;
  netif->hwaddr[2] = 0xC1;
  netif->hwaddr[3] = 0xDE;
  netif->hwaddr[4] = 0xD0;
  netif->hwaddr[5] = 0x0D;

  return ERR_OK;
}

static void dhcp_setup(void)
{
  txpacket = 0;
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void dhcp_teardown(void)
{
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void check_pkt(struct pbuf *p, u32_t pos, const u8_t *mem, u32_t len)
{
  u8_t *data;

  fail_if((pos + len) > p->tot_len);
  while (pos > p->len && p->next) {
    pos -= p->len;
    p = p->next;
  }
  fail_if(p == NULL);
  fail_unless(pos + len <= p->len); /* All data we seek within same pbuf */

  data = (u8_t*)p->payload;
  fail_if(memcmp(&data[pos], mem, len), "data at pos %d, len %d in packet %d did not match", pos, len, txpacket);
}

static void check_pkt_fuzzy(struct pbuf *p, u32_t startpos, const u8_t *mem, u32_t len)
{
  int found;
  u32_t i;
  u8_t *data;

  fail_if((startpos + len) > p->tot_len);
  while (startpos > p->len && p->next) {
    startpos -= p->len;
    p = p->next;
  }
  fail_if(p == NULL);
  fail_unless(startpos + len <= p->len); /* All data we seek within same pbuf */

  found = 0;
  data = (u8_t*)p->payload;
  for (i = startpos; i <= (p->len - len); i++) {
    if (memcmp(&data[i], mem, len) == 0) {
      found = 1;
      break;
    }
  }
  fail_unless(found);
}

static err_t lwip_tx_func(struct netif *netif, struct pbuf *p)
{
  fail_unless(netif == &net_test);
  txpacket++;

  if (debug) {
    struct pbuf *pp = p;
    /* Dump data */
    printf("TX data (pkt %d, len %d, tick %d)", txpacket, p->tot_len, tick);
    do {
      int i;
      for (i = 0; i < pp->len; i++) {
        printf(" %02X", ((u8_t *) pp->payload)[i]);
      }
      if (pp->next) {
        pp = pp->next;
      }
    } while (pp->next);
    printf("\n");
  }

  switch (tcase) {
  case TEST_LWIP_DHCP:
    switch (txpacket) {
    case 1:
    case 2:
      {
        const u8_t ipproto[] = { 0x08, 0x00 };
        const u8_t bootp_start[] = { 0x01, 0x01, 0x06, 0x00}; /* bootp request, eth, hwaddr len 6, 0 hops */
        const u8_t ipaddrs[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

        check_pkt(p, 0, broadcast, 6); /* eth level dest: broadcast */
        check_pkt(p, 6, netif->hwaddr, 6); /* eth level src: unit mac */

        check_pkt(p, 12, ipproto, sizeof(ipproto)); /* eth level proto: ip */

        check_pkt(p, 42, bootp_start, sizeof(bootp_start));

        check_pkt(p, 53, ipaddrs, sizeof(ipaddrs));

        check_pkt(p, 70, netif->hwaddr, 6); /* mac addr inside bootp */

        check_pkt(p, 278, magic_cookie, sizeof(magic_cookie));

        /* Check dchp message type, can be at different positions */
        if (txpacket == 1) {
          u8_t dhcp_discover_opt[] = { 0x35, 0x01, 0x01 };
          check_pkt_fuzzy(p, 282, dhcp_discover_opt, sizeof(dhcp_discover_opt));
        } else if (txpacket == 2) {
          u8_t dhcp_request_opt[] = { 0x35, 0x01, 0x03 };
          u8_t requested_ipaddr[] = { 0x32, 0x04, 0xc3, 0xaa, 0xbd, 0xc8 }; /* Ask for offered IP */

          check_pkt_fuzzy(p, 282, dhcp_request_opt, sizeof(dhcp_request_opt));
          check_pkt_fuzzy(p, 282, requested_ipaddr, sizeof(requested_ipaddr));
        }
        break;
      }
    case 3:
    case 4:
    case 5:
      {
        const u8_t arpproto[] = { 0x08, 0x06 };

        check_pkt(p, 0, broadcast, 6); /* eth level dest: broadcast */
        check_pkt(p, 6, netif->hwaddr, 6); /* eth level src: unit mac */

        check_pkt(p, 12, arpproto, sizeof(arpproto)); /* eth level proto: ip */
        break;
      }
      default:
        fail();
        break;
    }
    break;

  case TEST_LWIP_DHCP_NAK:
    {
      const u8_t ipproto[] = { 0x08, 0x00 };
      const u8_t bootp_start[] = { 0x01, 0x01, 0x06, 0x00}; /* bootp request, eth, hwaddr len 6, 0 hops */
      const u8_t ipaddrs[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
      const u8_t dhcp_nak_opt[] = { 0x35, 0x01, 0x04 };
      const u8_t requested_ipaddr[] = { 0x32, 0x04, 0xc3, 0xaa, 0xbd, 0xc8 }; /* offered IP */

      fail_unless(txpacket == 4);
      check_pkt(p, 0, broadcast, 6); /* eth level dest: broadcast */
      check_pkt(p, 6, netif->hwaddr, 6); /* eth level src: unit mac */

      check_pkt(p, 12, ipproto, sizeof(ipproto)); /* eth level proto: ip */

      check_pkt(p, 42, bootp_start, sizeof(bootp_start));

      check_pkt(p, 53, ipaddrs, sizeof(ipaddrs));

      check_pkt(p, 70, netif->hwaddr, 6); /* mac addr inside bootp */

      check_pkt(p, 278, magic_cookie, sizeof(magic_cookie));

      check_pkt_fuzzy(p, 282, dhcp_nak_opt, sizeof(dhcp_nak_opt)); /* NAK the ack */

      check_pkt_fuzzy(p, 282, requested_ipaddr, sizeof(requested_ipaddr));
      break;
    }

  case TEST_LWIP_DHCP_RELAY:
    switch (txpacket) {
    case 1:
    case 2:
      {
        const u8_t ipproto[] = { 0x08, 0x00 };
        const u8_t bootp_start[] = { 0x01, 0x01, 0x06, 0x00}; /* bootp request, eth, hwaddr len 6, 0 hops */
        const u8_t ipaddrs[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

        check_pkt(p, 0, broadcast, 6); /* eth level dest: broadcast */
        check_pkt(p, 6, netif->hwaddr, 6); /* eth level src: unit mac */

        check_pkt(p, 12, ipproto, sizeof(ipproto)); /* eth level proto: ip */

        check_pkt(p, 42, bootp_start, sizeof(bootp_start));

        check_pkt(p, 53, ipaddrs, sizeof(ipaddrs));

        check_pkt(p, 70, netif->hwaddr, 6); /* mac addr inside bootp */

        check_pkt(p, 278, magic_cookie, sizeof(magic_cookie));

        /* Check dchp message type, can be at different positions */
        if (txpacket == 1) {
          u8_t dhcp_discover_opt[] = { 0x35, 0x01, 0x01 };
          check_pkt_fuzzy(p, 282, dhcp_discover_opt, sizeof(dhcp_discover_opt));
        } else if (txpacket == 2) {
          u8_t dhcp_request_opt[] = { 0x35, 0x01, 0x03 };
          u8_t requested_ipaddr[] = { 0x32, 0x04, 0x4f, 0x8a, 0x33, 0x05 }; /* Ask for offered IP */

          check_pkt_fuzzy(p, 282, dhcp_request_opt, sizeof(dhcp_request_opt));
          check_pkt_fuzzy(p, 282, requested_ipaddr, sizeof(requested_ipaddr));
        }
        break;
      }
    case 3:
    case 4:
    case 5:
    case 6:
      {
        const u8_t arpproto[] = { 0x08, 0x06 };

        check_pkt(p, 0, broadcast, 6); /* eth level dest: broadcast */
        check_pkt(p, 6, netif->hwaddr, 6); /* eth level src: unit mac */

        check_pkt(p, 12, arpproto, sizeof(arpproto)); /* eth level proto: ip */
        break;
      }
    case 7:
      {
        const u8_t fake_arp[6] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xab };
        const u8_t ipproto[] = { 0x08, 0x00 };
        const u8_t bootp_start[] = { 0x01, 0x01, 0x06, 0x00}; /* bootp request, eth, hwaddr len 6, 0 hops */
        const u8_t ipaddrs[] = { 0x00, 0x4f, 0x8a, 0x33, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        const u8_t dhcp_request_opt[] = { 0x35, 0x01, 0x03 };

        check_pkt(p, 0, fake_arp, 6); /* eth level dest: broadcast */
        check_pkt(p, 6, netif->hwaddr, 6); /* eth level src: unit mac */

        check_pkt(p, 12, ipproto, sizeof(ipproto)); /* eth level proto: ip */

        check_pkt(p, 42, bootp_start, sizeof(bootp_start));

        check_pkt(p, 53, ipaddrs, sizeof(ipaddrs));

        check_pkt(p, 70, netif->hwaddr, 6); /* mac addr inside bootp */

        check_pkt(p, 278, magic_cookie, sizeof(magic_cookie));

        /* Check dchp message type, can be at different positions */
        check_pkt_fuzzy(p, 282, dhcp_request_opt, sizeof(dhcp_request_opt));
        break;
      }
    default:
      fail();
      break;
    }
    break;

  default:
    break;
  }

  return ERR_OK;
}

/*
 * Test basic happy flow DHCP session.
 * Validate that xid is checked.
 */
START_TEST(test_dhcp)
{
  ip4_addr_t addr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  int i;
  u32_t xid;
  LWIP_UNUSED_ARG(_i);

  tcase = TEST_LWIP_DHCP;
  setdebug(0);

  IP4_ADDR(&addr, 0, 0, 0, 0);
  IP4_ADDR(&netmask, 0, 0, 0, 0);
  IP4_ADDR(&gw, 0, 0, 0, 0);

  netif_add(&net_test, &addr, &netmask, &gw, &net_test, testif_init, ethernet_input);
  netif_set_link_up(&net_test);
  netif_set_up(&net_test);

  dhcp_start(&net_test);

  fail_unless(txpacket == 1); /* DHCP discover sent */
  xid = netif_dhcp_data(&net_test)->xid; /* Write bad xid, not using htonl! */
  memcpy(&dhcp_offer[46], &xid, 4);
  send_pkt(&net_test, dhcp_offer, sizeof(dhcp_offer));

  /* IP addresses should be zero */
  fail_if(memcmp(&addr, &net_test.ip_addr, sizeof(ip4_addr_t)));
  fail_if(memcmp(&netmask, &net_test.netmask, sizeof(ip4_addr_t)));
  fail_if(memcmp(&gw, &net_test.gw, sizeof(ip4_addr_t)));

  fail_unless(txpacket == 1, "TX %d packets, expected 1", txpacket); /* Nothing more sent */
  xid = htonl(netif_dhcp_data(&net_test)->xid);
  memcpy(&dhcp_offer[46], &xid, 4); /* insert correct transaction id */
  send_pkt(&net_test, dhcp_offer, sizeof(dhcp_offer));

  fail_unless(txpacket == 2, "TX %d packets, expected 2", txpacket); /* DHCP request sent */
  xid = netif_dhcp_data(&net_test)->xid; /* Write bad xid, not using htonl! */
  memcpy(&dhcp_ack[46], &xid, 4);
  send_pkt(&net_test, dhcp_ack, sizeof(dhcp_ack));

  fail_unless(txpacket == 2, "TX %d packets, still expected 2", txpacket); /* No more sent */
  xid = htonl(netif_dhcp_data(&net_test)->xid); /* xid updated */
  memcpy(&dhcp_ack[46], &xid, 4); /* insert transaction id */
  send_pkt(&net_test, dhcp_ack, sizeof(dhcp_ack));

  for (i = 0; i < 20; i++) {
    tick_lwip();
  }
  fail_unless(txpacket == 5, "TX %d packets, expected 5", txpacket); /* ARP requests sent */

  /* Interface up */
  fail_unless(netif_is_up(&net_test));

  /* Now it should have taken the IP */
  IP4_ADDR(&addr, 195, 170, 189, 200);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
  IP4_ADDR(&gw, 195, 170, 189, 171);
  fail_if(memcmp(&addr, &net_test.ip_addr, sizeof(ip4_addr_t)));
  fail_if(memcmp(&netmask, &net_test.netmask, sizeof(ip4_addr_t)));
  fail_if(memcmp(&gw, &net_test.gw, sizeof(ip4_addr_t)));

  tcase = TEST_NONE;
  dhcp_stop(&net_test);
  dhcp_cleanup(&net_test);
  netif_remove(&net_test);
}
END_TEST

/*
 * Test that IP address is not taken and NAK is sent if someone
 * replies to ARP requests for the offered address.
 */
START_TEST(test_dhcp_nak)
{
  ip4_addr_t addr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  u32_t xid;
  LWIP_UNUSED_ARG(_i);

  tcase = TEST_LWIP_DHCP;
  setdebug(0);

  IP4_ADDR(&addr, 0, 0, 0, 0);
  IP4_ADDR(&netmask, 0, 0, 0, 0);
  IP4_ADDR(&gw, 0, 0, 0, 0);

  netif_add(&net_test, &addr, &netmask, &gw, &net_test, testif_init, ethernet_input);
  netif_set_link_up(&net_test);
  netif_set_up(&net_test);

  dhcp_start(&net_test);

  fail_unless(txpacket == 1); /* DHCP discover sent */
  xid = netif_dhcp_data(&net_test)->xid; /* Write bad xid, not using htonl! */
  memcpy(&dhcp_offer[46], &xid, 4);
  send_pkt(&net_test, dhcp_offer, sizeof(dhcp_offer));

  /* IP addresses should be zero */
  fail_if(memcmp(&addr, &net_test.ip_addr, sizeof(ip4_addr_t)));
  fail_if(memcmp(&netmask, &net_test.netmask, sizeof(ip4_addr_t)));
  fail_if(memcmp(&gw, &net_test.gw, sizeof(ip4_addr_t)));

  fail_unless(txpacket == 1); /* Nothing more sent */
  xid = htonl(netif_dhcp_data(&net_test)->xid);
  memcpy(&dhcp_offer[46], &xid, 4); /* insert correct transaction id */
  send_pkt(&net_test, dhcp_offer, sizeof(dhcp_offer));

  fail_unless(txpacket == 2); /* DHCP request sent */
  xid = netif_dhcp_data(&net_test)->xid; /* Write bad xid, not using htonl! */
  memcpy(&dhcp_ack[46], &xid, 4);
  send_pkt(&net_test, dhcp_ack, sizeof(dhcp_ack));

  fail_unless(txpacket == 2); /* No more sent */
  xid = htonl(netif_dhcp_data(&net_test)->xid); /* xid updated */
  memcpy(&dhcp_ack[46], &xid, 4); /* insert transaction id */
  send_pkt(&net_test, dhcp_ack, sizeof(dhcp_ack));

  fail_unless(txpacket == 3); /* ARP request sent */

  tcase = TEST_LWIP_DHCP_NAK; /* Switch testcase */

  /* Send arp reply, mark offered IP as taken */
  send_pkt(&net_test, arpreply, sizeof(arpreply));

  fail_unless(txpacket == 4); /* DHCP nak sent */

  tcase = TEST_NONE;
  dhcp_stop(&net_test);
  dhcp_cleanup(&net_test);
  netif_remove(&net_test);
}
END_TEST

/*
 * Test case based on captured data where
 * replies are sent from a different IP than the
 * one the client unicasted to.
 */
START_TEST(test_dhcp_relayed)
{
  u8_t relay_offer[] = {
  0x00, 0x23, 0xc1, 0xde, 0xd0, 0x0d,
  0x00, 0x22, 0x93, 0x5a, 0xf7, 0x60,
  0x08, 0x00, 0x45, 0x00,
  0x01, 0x38, 0xfd, 0x53, 0x00, 0x00, 0x40, 0x11,
  0x78, 0x46, 0x4f, 0x8a, 0x32, 0x02, 0x4f, 0x8a,
  0x33, 0x05, 0x00, 0x43, 0x00, 0x44, 0x01, 0x24,
  0x00, 0x00, 0x02, 0x01, 0x06, 0x00, 0x51, 0x35,
  0xb6, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x4f, 0x8a, 0x33, 0x05, 0x00, 0x00,
  0x00, 0x00, 0x0a, 0xb5, 0x04, 0x01, 0x00, 0x23,
  0xc1, 0xde, 0xd0, 0x0d, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x82,
  0x53, 0x63, 0x01, 0x04, 0xff, 0xff, 0xfe, 0x00,
  0x03, 0x04, 0x4f, 0x8a, 0x32, 0x01, 0x06, 0x08,
  0x4f, 0x8a, 0x00, 0xb4, 0x55, 0x08, 0x1f, 0xd1,
  0x1c, 0x04, 0x4f, 0x8a, 0x33, 0xff, 0x33, 0x04,
  0x00, 0x00, 0x54, 0x49, 0x35, 0x01, 0x02, 0x36,
  0x04, 0x0a, 0xb5, 0x04, 0x01, 0xff
  };

  u8_t relay_ack1[] = {
  0x00, 0x23, 0xc1, 0xde, 0xd0, 0x0d, 0x00, 0x22,
  0x93, 0x5a, 0xf7, 0x60, 0x08, 0x00, 0x45, 0x00,
  0x01, 0x38, 0xfd, 0x55, 0x00, 0x00, 0x40, 0x11,
  0x78, 0x44, 0x4f, 0x8a, 0x32, 0x02, 0x4f, 0x8a,
  0x33, 0x05, 0x00, 0x43, 0x00, 0x44, 0x01, 0x24,
  0x00, 0x00, 0x02, 0x01, 0x06, 0x00, 0x51, 0x35,
  0xb6, 0xa1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x4f, 0x8a, 0x33, 0x05, 0x00, 0x00,
  0x00, 0x00, 0x0a, 0xb5, 0x04, 0x01, 0x00, 0x23,
  0xc1, 0xde, 0xd0, 0x0d, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x82,
  0x53, 0x63, 0x01, 0x04, 0xff, 0xff, 0xfe, 0x00,
  0x03, 0x04, 0x4f, 0x8a, 0x32, 0x01, 0x06, 0x08,
  0x4f, 0x8a, 0x00, 0xb4, 0x55, 0x08, 0x1f, 0xd1,
  0x1c, 0x04, 0x4f, 0x8a, 0x33, 0xff, 0x33, 0x04,
  0x00, 0x00, 0x54, 0x49, 0x35, 0x01, 0x05, 0x36,
  0x04, 0x0a, 0xb5, 0x04, 0x01, 0xff
  };

  u8_t relay_ack2[] = {
  0x00, 0x23, 0xc1, 0xde, 0xd0, 0x0d,
  0x00, 0x22, 0x93, 0x5a, 0xf7, 0x60,
  0x08, 0x00, 0x45, 0x00,
  0x01, 0x38, 0xfa, 0x18, 0x00, 0x00, 0x40, 0x11,
  0x7b, 0x81, 0x4f, 0x8a, 0x32, 0x02, 0x4f, 0x8a,
  0x33, 0x05, 0x00, 0x43, 0x00, 0x44, 0x01, 0x24,
  0x00, 0x00, 0x02, 0x01, 0x06, 0x00, 0x49, 0x8b,
  0x6e, 0xab, 0x00, 0x00, 0x00, 0x00, 0x4f, 0x8a,
  0x33, 0x05, 0x4f, 0x8a, 0x33, 0x05, 0x00, 0x00,
  0x00, 0x00, 0x0a, 0xb5, 0x04, 0x01, 0x00, 0x23,
  0xc1, 0xde, 0xd0, 0x0d, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x82,
  0x53, 0x63, 0x01, 0x04, 0xff, 0xff, 0xfe, 0x00,
  0x03, 0x04, 0x4f, 0x8a, 0x32, 0x01, 0x06, 0x08,
  0x4f, 0x8a, 0x00, 0xb4, 0x55, 0x08, 0x1f, 0xd1,
  0x1c, 0x04, 0x4f, 0x8a, 0x33, 0xff, 0x33, 0x04,
  0x00, 0x00, 0x54, 0x60, 0x35, 0x01, 0x05, 0x36,
  0x04, 0x0a, 0xb5, 0x04, 0x01, 0xff };

  const u8_t arp_resp[] = {
  0x00, 0x23, 0xc1, 0xde, 0xd0, 0x0d, /* DEST */
  0x00, 0x22, 0x93, 0x5a, 0xf7, 0x60, /* SRC */
  0x08, 0x06, /* Type: ARP */
  0x00, 0x01, /* HW: Ethernet */
  0x08, 0x00, /* PROTO: IP */
  0x06, /* HW size */
  0x04, /* PROTO size */
  0x00, 0x02, /* OPCODE: Reply */

  0x12, 0x34, 0x56, 0x78, 0x9a, 0xab, /* Target MAC */
  0x4f, 0x8a, 0x32, 0x01, /* Target IP */

  0x00, 0x23, 0xc1, 0x00, 0x06, 0x50, /* src mac */
  0x4f, 0x8a, 0x33, 0x05, /* src ip */

  /* Padding follows.. */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00 };

  ip4_addr_t addr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  int i;
  u32_t xid;
  LWIP_UNUSED_ARG(_i);

  tcase = TEST_LWIP_DHCP_RELAY;
  setdebug(0);

  IP4_ADDR(&addr, 0, 0, 0, 0);
  IP4_ADDR(&netmask, 0, 0, 0, 0);
  IP4_ADDR(&gw, 0, 0, 0, 0);

  netif_add(&net_test, &addr, &netmask, &gw, &net_test, testif_init, ethernet_input);
  netif_set_link_up(&net_test);
  netif_set_up(&net_test);

  dhcp_start(&net_test);

  fail_unless(txpacket == 1); /* DHCP discover sent */

  /* IP addresses should be zero */
  fail_if(memcmp(&addr, &net_test.ip_addr, sizeof(ip4_addr_t)));
  fail_if(memcmp(&netmask, &net_test.netmask, sizeof(ip4_addr_t)));
  fail_if(memcmp(&gw, &net_test.gw, sizeof(ip4_addr_t)));

  fail_unless(txpacket == 1); /* Nothing more sent */
  xid = htonl(netif_dhcp_data(&net_test)->xid);
  memcpy(&relay_offer[46], &xid, 4); /* insert correct transaction id */
  send_pkt(&net_test, relay_offer, sizeof(relay_offer));

  /* request sent? */
  fail_unless(txpacket == 2, "txpkt = %d, should be 2", txpacket);
  xid = htonl(netif_dhcp_data(&net_test)->xid); /* xid updated */
  memcpy(&relay_ack1[46], &xid, 4); /* insert transaction id */
  send_pkt(&net_test, relay_ack1, sizeof(relay_ack1));

  for (i = 0; i < 25; i++) {
    tick_lwip();
  }
  fail_unless(txpacket == 5, "txpkt should be 5, is %d", txpacket); /* ARP requests sent */

  /* Interface up */
  fail_unless(netif_is_up(&net_test));

  /* Now it should have taken the IP */
  IP4_ADDR(&addr, 79, 138, 51, 5);
  IP4_ADDR(&netmask, 255, 255, 254, 0);
  IP4_ADDR(&gw, 79, 138, 50, 1);
  fail_if(memcmp(&addr, &net_test.ip_addr, sizeof(ip4_addr_t)));
  fail_if(memcmp(&netmask, &net_test.netmask, sizeof(ip4_addr_t)));
  fail_if(memcmp(&gw, &net_test.gw, sizeof(ip4_addr_t)));

  fail_unless(txpacket == 5, "txpacket = %d", txpacket);

  for (i = 0; i < 108000 - 25; i++) {
    tick_lwip();
  }

  fail_unless(netif_is_up(&net_test));
  fail_unless(txpacket == 6, "txpacket = %d", txpacket);

  /* We need to send arp response here.. */

  send_pkt(&net_test, arp_resp, sizeof(arp_resp));

  fail_unless(txpacket == 7, "txpacket = %d", txpacket);
  fail_unless(netif_is_up(&net_test));

  xid = htonl(netif_dhcp_data(&net_test)->xid); /* xid updated */
  memcpy(&relay_ack2[46], &xid, 4); /* insert transaction id */
  send_pkt(&net_test, relay_ack2, sizeof(relay_ack2));

  for (i = 0; i < 100000; i++) {
    tick_lwip();
  }

  fail_unless(txpacket == 7, "txpacket = %d", txpacket);

  tcase = TEST_NONE;
  dhcp_stop(&net_test);
  dhcp_cleanup(&net_test);
  netif_remove(&net_test);

}
END_TEST

START_TEST(test_dhcp_nak_no_endmarker)
{
  ip4_addr_t addr;
  ip4_addr_t netmask;
  ip4_addr_t gw;

  u8_t dhcp_nack_no_endmarker[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x54, 0x75,
    0xd0, 0x26, 0xd0, 0x0d, 0x08, 0x00, 0x45, 0x00,
    0x01, 0x15, 0x38, 0x86, 0x00, 0x00, 0xff, 0x11,
    0xc0, 0xa8, 0xc0, 0xa8, 0x01, 0x01, 0xff, 0xff,
    0xff, 0xff, 0x00, 0x43, 0x00, 0x44, 0x01, 0x01,
    0x00, 0x00, 0x02, 0x01, 0x06, 0x00, 0x7a, 0xcb,
    0xba, 0xf2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23,
    0xc1, 0xde, 0xd0, 0x0d, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x82,
    0x53, 0x63, 0x35, 0x01, 0x06, 0x36, 0x04, 0xc0,
    0xa8, 0x01, 0x01, 0x31, 0xef, 0xad, 0x72, 0x31,
    0x43, 0x4e, 0x44, 0x30, 0x32, 0x35, 0x30, 0x43,
    0x52, 0x47, 0x44, 0x38, 0x35, 0x36, 0x3c, 0x08,
    0x4d, 0x53, 0x46, 0x54, 0x20, 0x35, 0x2e, 0x30,
    0x37, 0x0d, 0x01, 0x0f, 0x03, 0x06, 0x2c, 0x2e,
    0x2f, 0x1f, 0x21, 0x79, 0xf9, 0x2b, 0xfc, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe2, 0x71,
    0xf3, 0x5b, 0xe2, 0x71, 0x2e, 0x01, 0x08, 0x03,
    0x04, 0xc0, 0xa8, 0x01, 0x01, 0xff, 0xeb, 0x1e,
    0x44, 0xec, 0xeb, 0x1e, 0x30, 0x37, 0x0c, 0x01,
    0x0f, 0x03, 0x06, 0x2c, 0x2e, 0x2f, 0x1f, 0x21,
    0x79, 0xf9, 0x2b, 0xff, 0x25, 0xc0, 0x09, 0xd6,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  u32_t xid;
  struct dhcp* dhcp;
  u8_t tries;
  u16_t request_timeout;
  LWIP_UNUSED_ARG(_i);

  tcase = TEST_LWIP_DHCP_NAK_NO_ENDMARKER;
  setdebug(0);

  IP4_ADDR(&addr, 0, 0, 0, 0);
  IP4_ADDR(&netmask, 0, 0, 0, 0);
  IP4_ADDR(&gw, 0, 0, 0, 0);

  netif_add(&net_test, &addr, &netmask, &gw, &net_test, testif_init, ethernet_input);
  netif_set_link_up(&net_test);
  netif_set_up(&net_test);

  dhcp_start(&net_test);
  dhcp = netif_dhcp_data(&net_test);

  fail_unless(txpacket == 1); /* DHCP discover sent */
  xid = dhcp->xid; /* Write bad xid, not using htonl! */
  memcpy(&dhcp_offer[46], &xid, 4);
  send_pkt(&net_test, dhcp_offer, sizeof(dhcp_offer));

  /* IP addresses should be zero */
  fail_if(memcmp(&addr, &net_test.ip_addr, sizeof(ip4_addr_t)));
  fail_if(memcmp(&netmask, &net_test.netmask, sizeof(ip4_addr_t)));
  fail_if(memcmp(&gw, &net_test.gw, sizeof(ip4_addr_t)));

  fail_unless(txpacket == 1); /* Nothing more sent */
  xid = htonl(dhcp->xid);
  memcpy(&dhcp_offer[46], &xid, 4); /* insert correct transaction id */
  send_pkt(&net_test, dhcp_offer, sizeof(dhcp_offer));

  fail_unless(dhcp->state == DHCP_STATE_REQUESTING);

  fail_unless(txpacket == 2); /* No more sent */
  xid = htonl(dhcp->xid); /* xid updated */
  memcpy(&dhcp_nack_no_endmarker[46], &xid, 4); /* insert transaction id */
  tries = dhcp->tries;
  request_timeout = dhcp->request_timeout;
  send_pkt(&net_test, dhcp_nack_no_endmarker, sizeof(dhcp_nack_no_endmarker));

  /* NAK should be ignored */
  fail_unless(dhcp->state == DHCP_STATE_REQUESTING);
  fail_unless(txpacket == 2); /* No more sent */
  fail_unless(xid == htonl(dhcp->xid));
  fail_unless(tries == dhcp->tries);
  fail_unless(request_timeout == dhcp->request_timeout);

  tcase = TEST_NONE;
  dhcp_stop(&net_test);
  dhcp_cleanup(&net_test);
  netif_remove(&net_test);
}
END_TEST

START_TEST(test_dhcp_invalid_overload)
{
  u8_t dhcp_offer_invalid_overload[] = {
      0x00, 0x23, 0xc1, 0xde, 0xd0, 0x0d, /* To unit */
      0x00, 0x0F, 0xEE, 0x30, 0xAB, 0x22, /* From Remote host */
      0x08, 0x00, /* Protocol: IP */
      0x45, 0x10, 0x01, 0x48, 0x00, 0x00, 0x00, 0x00, 0x80, 0x11, 0x36, 0xcc, 0xc3, 0xaa, 0xbd, 0xab, 0xc3, 0xaa, 0xbd, 0xc8, /* IP header */
      0x00, 0x43, 0x00, 0x44, 0x01, 0x34, 0x00, 0x00, /* UDP header */

      0x02, /* Type == Boot reply */
      0x01, 0x06, /* Hw Ethernet, 6 bytes addrlen */
      0x00, /* 0 hops */
      0xAA, 0xAA, 0xAA, 0xAA, /* Transaction id, will be overwritten */
      0x00, 0x00, /* 0 seconds elapsed */
      0x00, 0x00, /* Flags (unicast) */
      0x00, 0x00, 0x00, 0x00, /* Client ip */
      0xc3, 0xaa, 0xbd, 0xc8, /* Your IP */
      0xc3, 0xaa, 0xbd, 0xab, /* DHCP server ip */
      0x00, 0x00, 0x00, 0x00, /* relay agent */
      0x00, 0x23, 0xc1, 0xde, 0xd0, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* MAC addr + padding */

      /* Empty server name */
      0x34, 0x01, 0x02, 0xff, /* Overload: SNAME + END */
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      /* Empty boot file name */
      0x34, 0x01, 0x01, 0xff, /* Overload FILE + END */
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x63, 0x82, 0x53, 0x63, /* Magic cookie */
      0x35, 0x01, 0x02, /* Message type: Offer */
      0x36, 0x04, 0xc3, 0xaa, 0xbd, 0xab, /* Server identifier (IP) */
      0x33, 0x04, 0x00, 0x00, 0x00, 0x78, /* Lease time 2 minutes */
      0x03, 0x04, 0xc3, 0xaa, 0xbd, 0xab, /* Router IP */
      0x01, 0x04, 0xff, 0xff, 0xff, 0x00, /* Subnet mask */
      0x34, 0x01, 0x03, /* Overload: FILE + SNAME */
      0xff, /* End option */
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Padding */
  };
  ip4_addr_t addr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  u32_t xid;
  LWIP_UNUSED_ARG(_i);

  tcase = TEST_LWIP_DHCP_INVALID_OVERLOAD;
  setdebug(0);

  IP4_ADDR(&addr, 0, 0, 0, 0);
  IP4_ADDR(&netmask, 0, 0, 0, 0);
  IP4_ADDR(&gw, 0, 0, 0, 0);

  netif_add(&net_test, &addr, &netmask, &gw, &net_test, testif_init, ethernet_input);
  netif_set_link_up(&net_test);
  netif_set_up(&net_test);

  dhcp_start(&net_test);

  fail_unless(txpacket == 1); /* DHCP discover sent */
  xid = htonl(netif_dhcp_data(&net_test)->xid);
  memcpy(&dhcp_offer_invalid_overload[46], &xid, 4); /* insert correct transaction id */
  dhcp_offer_invalid_overload[311] = 3;
  send_pkt(&net_test, dhcp_offer_invalid_overload, sizeof(dhcp_offer_invalid_overload));
  /* IP addresses should be zero */
  fail_if(memcmp(&addr, &net_test.ip_addr, sizeof(ip4_addr_t)));
  fail_if(memcmp(&netmask, &net_test.netmask, sizeof(ip4_addr_t)));
  fail_if(memcmp(&gw, &net_test.gw, sizeof(ip4_addr_t)));
  fail_unless(txpacket == 1); /* Nothing more sent */

  dhcp_offer_invalid_overload[311] = 2;
  send_pkt(&net_test, dhcp_offer_invalid_overload, sizeof(dhcp_offer_invalid_overload));
  /* IP addresses should be zero */
  fail_if(memcmp(&addr, &net_test.ip_addr, sizeof(ip4_addr_t)));
  fail_if(memcmp(&netmask, &net_test.netmask, sizeof(ip4_addr_t)));
  fail_if(memcmp(&gw, &net_test.gw, sizeof(ip4_addr_t)));
  fail_unless(txpacket == 1); /* Nothing more sent */

  dhcp_offer_invalid_overload[311] = 1;
  send_pkt(&net_test, dhcp_offer_invalid_overload, sizeof(dhcp_offer_invalid_overload));
  /* IP addresses should be zero */
  fail_if(memcmp(&addr, &net_test.ip_addr, sizeof(ip4_addr_t)));
  fail_if(memcmp(&netmask, &net_test.netmask, sizeof(ip4_addr_t)));
  fail_if(memcmp(&gw, &net_test.gw, sizeof(ip4_addr_t)));
  fail_unless(txpacket == 1); /* Nothing more sent */

  dhcp_offer_invalid_overload[311] = 0;
  send_pkt(&net_test, dhcp_offer_invalid_overload, sizeof(dhcp_offer));

  fail_unless(netif_dhcp_data(&net_test)->state == DHCP_STATE_REQUESTING);

  fail_unless(txpacket == 2); /* No more sent */
  xid = htonl(netif_dhcp_data(&net_test)->xid); /* xid updated */

  tcase = TEST_NONE;
  dhcp_stop(&net_test);
  dhcp_cleanup(&net_test);
  netif_remove(&net_test);
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
dhcp_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_dhcp),
    TESTFUNC(test_dhcp_nak),
    TESTFUNC(test_dhcp_relayed),
    TESTFUNC(test_dhcp_nak_no_endmarker),
    TESTFUNC(test_dhcp_invalid_overload)
  };
  return create_suite("DHCP", tests, sizeof(tests)/sizeof(testfunc), dhcp_setup, dhcp_teardown);
}
