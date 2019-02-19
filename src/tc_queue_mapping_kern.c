/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/bpf.h>
#include <linux/pkt_cls.h>
#include "bpf_helpers.h"

/* Manuel setup:

 tc qdisc  add dev ixgbe2 clsact
 tc filter add dev ixgbe2 egress bpf da obj tc_queue_mapping_kern.o sec tc_qmap2cpu
 tc filter list dev ixgbe2 egress

*/

//#define DEBUG 1
#ifdef  DEBUG
/* Only use this for debug output. Notice output from bpf_trace_printk()
 * end-up in /sys/kernel/debug/tracing/trace_pipe
 */
#define bpf_debug(fmt, ...)                                             \
                ({                                                      \
                        char ____fmt[] = fmt;                           \
                        bpf_trace_printk(____fmt, sizeof(____fmt),      \
                                     ##__VA_ARGS__);                    \
                })
#else
#define bpf_debug(fmt, ...) { } while (0)
#endif

SEC("tc_qmap2cpu")
int  tc_cls_prog(struct __sk_buff *skb)
{
	__u32 cpu = bpf_get_smp_processor_id();
	__u16 txq_root_handle;

	/* The skb->queue_mapping is 1-indexed (zero means queue_mapping not
	 * set).  The underlying MQ leaf's are also 1-indexed, which makes it
	 * easier to reason about.
	 */
	txq_root_handle = cpu + 1;
	skb->queue_mapping = txq_root_handle;

	/* Details: Kernel double protect against setting a too high
	 * queue_mapping.  In skb_tx_hash() it will reduce number to be
	 * less-than (or equal) dev->real_num_tx_queues.  And netdev_pick_tx()
	 * cap via netdev_cap_txqueue().
	 */
/*
  Do simple mapping of CPU to queue_mapping.
  -----------------------------------------
  Assuming MQ is created with handle 7FFF:
    tc qdisc replace dev ixgbe2 root handle 7FFF: mq

  And for each MQ-leaf HTBs are created
    # Foreach TXQ - create HTB leaf(s) under MQ 0x7FFF:TXQ
    tc qdisc add dev ixgbe2 parent 7FFF:1 handle 1: htb default 2
    tc qdisc add dev ixgbe2 parent 7FFF:2 handle 2: htb default 2
    tc qdisc add dev ixgbe2 parent 7FFF:3 handle 3: htb default 2
    tc qdisc add dev ixgbe2 parent 7FFF:4 handle 4: htb default 2

  Gives the following mapping table:
  |-----+---------------+---------+-----------|
  | CPU | queue_mapping | MQ-leaf | HTB major |
  |-----+---------------+---------+-----------|
  |   0 |             1 | 7FFF:1  |        1: |
  |   1 |             2 | 7FFF:2  |        2: |
  |   2 |             3 | 7FFF:3  |        3: |
  |   3 |             4 | 7FFF:4  |        4: |
  |-----+---------------+---------+-----------|

*/
	bpf_debug("hello cpu:%d queue_mapping:%d \n", cpu, skb->queue_mapping);

	return TC_ACT_OK;;
}

char _license[] SEC("license") = "GPL";
