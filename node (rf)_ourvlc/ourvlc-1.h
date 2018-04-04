#ifndef _OURVLC_H
#define _OURVLC_H

#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/netdevice.h>   /* struct device, and other headers */
#include <linux/if_arp.h>		// ARPHRD_LOOPBACK
#include <linux/skbuff.h>
#include <linux/kthread.h>		// kthread_run
#include <asm/io.h>		// ioremap

#include <linux/proc_fs.h> // proc
#include <linux/ioctl.h>
#include <rtdm/rtdm_driver.h> // rtdm timer
#include <linux/hrtimer.h> // dependencies for rtdm timer (maybe)
#include <linux/ktime.h>
#include <linux/delay.h>

#include <linux/rslib.h>

#define ADDRESS_LENGTH 2
#define SEQ_NUMBER_LENGTH 4
#define ECC_LEN	16					// [TODO] Change to 2 when receiving packet from kernel is already implemented
#define OTHER_PACKET_CONTENT_BYTE (ADDRESS_LENGTH * 2 + SEQ_NUMBER_LENGTH)
#define BUFFER_LIMIT 6004
#define MTU_VALUE 150

#define PRU_ADDR 0x4a300000

#define MAC_ADDR_LEN 2
#define PROTOCOL_LEN 2
#define MAC_HDR_LEN (OCTET_LEN+2*MAC_ADDR_LEN+PROTOCOL_LEN)
#define VLC_HLEN (2*MAC_ADDR_LEN+PROTOCOL_LEN)

#define RX_DATA_LEN 2000

#define DUMMY_PACKET_NODE_ID 99

#define VLC_P_DEFAULT     0x0001          /* Dummy type for vlc frames  */

struct vlchdr {
	unsigned char	h_dest[MAC_ADDR_LEN];	/* destination eth addr	*/
	unsigned char	h_source[MAC_ADDR_LEN];	/* source ether addr	*/
	__be16		h_proto;		/* packet type ID field	*/
} __attribute__((packed));

#endif
