/*
 *  ourvlc-1.c - The simplest kernel module.
 */
#include "ourvlc-1.h"

MODULE_AUTHOR("Yogi Salomo");
MODULE_LICENSE("Dual BSD/GPL");

struct net_device *vlc_devs;


struct vlc_packet *rx_pkt;
struct vlc_packet *rs_error_pkt;
//struct vlc_packet *rx_pkt_check_dup;


static struct task_struct *task_phy_decoding = NULL;

static int mtu = 100;

static int flag_exit = 0;

volatile void* pru0;

static uint8_t rx_data[RX_DATA_LEN]; // Length > MAC_HDR_LEN+maxpayload+rs_code;

struct proc_dir_entry *vlc_dir, *tx_device, *rx_device;
static int rx_device_value = 0;
static int tx_device_value = 0;

static int block_size = 200;
module_param(block_size, int, 0);

uint16_t par[ECC_LEN];
int ecc_symsize = 8;
int ecc_poly = 0x11d;

/* the Reed Solomon control structure */
static struct rs_control *rs_decoder;

uint16_t error_node_id = 98;

//static rtdm_timer_t phy_timer;
//uint32_t slot_ns;

//uint16_t read_idx = 0, write_idx = 0, datalen = 0;
//int read_idx_storage_offset = 2;
//int write_idx_storage_offset = 0;

struct vlc_priv {
	struct net_device_stats stats;
	int status;
	struct vlc_packet *ppool;
	int rx_int_enabled;
	struct vlc_packet *tx_queue; // List of tx packets
	int tx_packetlen;
	u8 *tx_packetdata;
	struct sk_buff *skb;
	spinlock_t lock;
    struct net_device *dev;
};

struct vlc_packet {
	struct vlc_packet *next;
	struct net_device *dev;
	int	datalen;
	u8 data[2000+10];
};

static struct vlchdr *vlc_hdr(const struct sk_buff *skb)
{
    return (struct vlchdr *) skb_mac_header(skb);
}

__be16 vlc_type_trans(struct sk_buff *skb, struct net_device *dev)
{
   struct vlchdr *vlc;
   skb->dev = dev;
   skb_reset_mac_header(skb);
   skb_pull_inline(skb, VLC_HLEN);
   vlc = vlc_hdr(skb);

   if (ntohs(vlc->h_proto) >= 1536)
       return vlc->h_proto;

   return htons(VLC_P_DEFAULT);
}

int pool_size = 1000;
module_param(pool_size, int, 0);

static void vlc_rx_ints(struct net_device *dev, int enable)
{
	struct vlc_priv *priv = netdev_priv(dev);
	priv->rx_int_enabled = enable;
}

// Setup VLC network device
void vlc_setup(struct net_device *dev)
{
    dev->hard_header_len = VLC_HLEN;
    dev->mtu = mtu;
    dev->tx_queue_len = 100;
    dev->priv_flags |= IFF_TX_SKB_SHARING;
}

void vlc_setup_pool(struct net_device *dev)
{
	struct vlc_priv *priv = netdev_priv(dev);
	int i;
	struct vlc_packet *pkt;

	priv->ppool = NULL;
	for (i = 0; i < pool_size; i++) {
		pkt = kmalloc (sizeof (struct vlc_packet), GFP_KERNEL);
		if (pkt == NULL) {
			printk (KERN_NOTICE "Ran out of memory allocating packet pool\n");
			return;
		}
		pkt->dev = dev;
		pkt->next = priv->ppool;
		priv->ppool = pkt;
	}
}

void vlc_teardown_pool(struct net_device *dev)
{
	struct vlc_priv *priv = netdev_priv(dev);
	struct vlc_packet *pkt;
    //unsigned long flags;

    //spin_lock_bh(&priv->lock);
	while ((pkt = priv->ppool)) {
		priv->ppool = pkt->next;
		if (pkt) kfree (pkt);
		/* FIXME - in-flight packets ? */
	}
    //spin_unlock_bh(&priv->lock);
}

int vlc_open(struct net_device *dev)
{
	memcpy(dev->dev_addr, "\0\2", MAC_ADDR_LEN);
    netif_start_queue(dev);
	return 0;
}

int vlc_release(struct net_device *dev)
{
	//netif_stop_queue(dev); /* can't transmit any more */
	return 0;
}

int vlc_tx(struct sk_buff *skb, struct net_device *dev)
{
	return 0; /* Our simple device can not fail */
}

int vlc_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	//printk("ioctl\n");
	return 0;
}

int vlc_config(struct net_device *dev, struct ifmap *map)
{
	if (dev->flags & IFF_UP) /* can't act on a running interface */
		return -EBUSY;

	/* Don't allow changing the I/O address */
	if (map->base_addr != dev->base_addr) {
		printk(KERN_WARNING "snull: Can't change I/O address\n");
		return -EOPNOTSUPP;
	}

	/* Allow changing the IRQ */
	if (map->irq != dev->irq) {
		dev->irq = map->irq; /* request_irq() is delayed to open-time */
	}

	/* ignore other fields */
	return 0;
}

struct net_device_stats *vlc_stats(struct net_device *dev)
{
	struct vlc_priv *priv = netdev_priv(dev);
	return &priv->stats;
}

static const struct net_device_ops vlc_netdev_ops = {
	.ndo_open            = vlc_open,
	.ndo_stop            = vlc_release,
	.ndo_start_xmit      = vlc_tx,
	.ndo_do_ioctl        = vlc_ioctl,
	.ndo_set_config      = vlc_config,
	.ndo_get_stats       = vlc_stats,
};

int vlc_header(struct sk_buff *skb, struct net_device *dev, unsigned short type,
                const void *daddr, const void *saddr, unsigned len)
{
	struct vlchdr *vlc = (struct vlchdr *)skb_push(skb, VLC_HLEN);

	vlc->h_proto = htons(type);
	memcpy(vlc->h_source, saddr ? saddr : dev->dev_addr, dev->addr_len);
	memcpy(vlc->h_dest,   daddr ? daddr : dev->dev_addr, dev->addr_len);
	vlc->h_dest[MAC_ADDR_LEN-1] ^= 0x01;   /* dest is us xor 1 */
	return (dev->hard_header_len);
}

int vlc_rebuild_header(struct sk_buff *skb)
{
	struct vlchdr *vlc = (struct vlchdr *) skb->data;
	struct net_device *dev = skb->dev;

	memcpy(vlc->h_source, dev->dev_addr, dev->addr_len);
	memcpy(vlc->h_dest, dev->dev_addr, dev->addr_len);
	vlc->h_dest[MAC_ADDR_LEN-1] ^= 0x01;   /* dest is us xor 1 */
	return 0;
}


static const struct header_ops vlc_header_ops= {
    .create  = vlc_header,
	.rebuild = vlc_rebuild_header
};


int proc_read(char *page, char **start, off_t off, int count, int *eof,
                void *data)
{
    count = sprintf(page, "%d", *(int *)data);
    return count;
}

int proc_write(struct file *file, const char *buffer,
    unsigned long count, void *data)
{
    unsigned int c = 0, len = 0, val, sum = 0;
    int * temp = (int *)data;

    while (count) {
        if (get_user(c, buffer))
            return -EFAULT;

        len++;
        buffer++;
        count--;

        if (c == 10 || c == 0)
            break;
        val = c - '0';
        if (val > 9)
            return -EINVAL;
        sum *= 10;
        sum += val;
    }
    *temp = sum;
    return len;
}

void vlc_init(struct net_device *dev)
{
	struct vlc_priv *priv;

	dev->addr_len = MAC_ADDR_LEN;
    dev->type = ARPHRD_LOOPBACK ;  // ARPHRD_IEEE802154
    vlc_setup(dev); /* assign some of the fields */
	dev->netdev_ops = &vlc_netdev_ops;
	dev->header_ops = &vlc_header_ops;
	/* keep the default flags, just add NOARP */
	dev->flags           |= IFF_NOARP;
	dev->features        |= NETIF_F_HW_CSUM;

	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct vlc_priv));
	vlc_rx_ints(dev, 1);		/* enable receive interrupts */
	rx_pkt = kmalloc (sizeof (struct vlc_packet), GFP_KERNEL);
	rs_error_pkt = kmalloc(sizeof(struct vlc_packet), GFP_KERNEL);
//	rx_pkt_check_dup = kmalloc (sizeof (struct vlc_packet), GFP_KERNEL);
//	if (rx_pkt_check_dup==NULL || rx_pkt==NULL) {
//		printk (KERN_NOTICE "Ran out of memory allocating packet pool\n");
//		return ;
//	}
//
//	rx_pkt_check_dup->datalen = 0;
	vlc_setup_pool(dev);
	priv->tx_queue = NULL;
	flag_exit = 0;
}

void mac_rx(struct net_device *dev, struct vlc_packet *pkt)
{
	struct sk_buff *skb;
	struct vlc_priv *priv = netdev_priv(dev);

	skb = dev_alloc_skb(pkt->datalen + 2);
	if (!skb) {
		if (printk_ratelimit())
			printk(KERN_NOTICE "snull rx: low on mem - packet dropped\n");
		priv->stats.rx_dropped++;
		goto out;
	}
	skb_reserve(skb, 2); /* align IP on 16B boundary */
	memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);

	/* Write metadata, and then pass to the receive level */
	skb->dev = dev;
	skb->protocol = vlc_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	priv->stats.tx_packets++;
	priv->stats.tx_bytes += pkt->datalen;
	netif_rx(skb);
//	printk("after netif rx\n");
	//kfree(pkt);
  out:
	return;
}

static int pru_phy_decoding(void *data){
//	struct vlc_priv *priv = netdev_priv(vlc_devs);
	uint16_t read_idx = 0, write_idx = 0, datalen = 0;
	int read_idx_storage_offset = 2;
	int write_idx_storage_offset = 0;
	int index_block, num_of_blocks;
	int i;
	unsigned int num_err = 0, total_num_err = 0;
	uint16_t curr_pkt_node_id;

	for(;;){
		read_idx = readw(pru0+read_idx_storage_offset);
		write_idx = readw(pru0+write_idx_storage_offset);

		if(read_idx == write_idx){
			msleep(1);
			continue;
		}

		datalen = readw(pru0+read_idx);

		if(datalen > 0 && datalen<= MTU_VALUE){

			memset(rx_data,0,sizeof(uint8_t) * RX_DATA_LEN);

			datalen += (OTHER_PACKET_CONTENT_BYTE + sizeof(uint16_t));
			if (datalen % block_size)
				num_of_blocks = datalen / block_size + 1;
			else
				num_of_blocks = datalen / block_size;

			memcpy_fromio(rx_data, pru0+read_idx,(datalen+ECC_LEN));

			total_num_err = 0;
			for (index_block=0; index_block < num_of_blocks; index_block++) {
				for (i = 0; i < ECC_LEN; i++) {
					par[i] = rx_data[datalen+index_block*ECC_LEN+i];
					//printk(" %02x", par[i]);
				}
				//printk("\n");
				if (index_block < num_of_blocks-1) {
					num_err = decode_rs8(rs_decoder, rx_data+index_block*block_size,
						par, block_size, NULL, 0, NULL, 0, NULL);
				} else { // The last block
					num_err = decode_rs8(rs_decoder, rx_data+index_block*block_size,
						par, datalen % block_size, NULL, 0, NULL, 0, NULL);
				}

				if (num_err < 0) {
//					printk("*** CRC error. ***\n");
					return -1;
				}
				total_num_err += num_err;
			}

			if (total_num_err > 0) {
				memcpy((u8*)&curr_pkt_node_id,&rx_data[2],sizeof(uint16_t));
				if(curr_pkt_node_id != DUMMY_PACKET_NODE_ID){
					memcpy(&rx_data[2],(u8*)&error_node_id,sizeof(uint16_t));
				}
//				printk("*** %d bytes are corrected. ***\n",total_num_err);
//				printk("%d %d is the new node id\n",rx_pkt->data[2],rx_pkt->data[3]);
				memcpy(&rx_data[10],(u8*)&total_num_err,sizeof(unsigned int));
			}

			rx_pkt->datalen = datalen;
			memcpy(rx_pkt->data,rx_data, rx_pkt->datalen);
			mac_rx(vlc_devs,rx_pkt);
//			if(printk_ratelimit()){
//				printk("Read IDX: %d",read_idx);
//			}
			read_idx += (datalen+ ECC_LEN);
			if(read_idx >= BUFFER_LIMIT){
				read_idx = 4;
			}
			writew(read_idx,pru0+read_idx_storage_offset);
		}

		if (kthread_should_stop()){
			return 0;
		}

		udelay(125);
	}

	return 0;
}

//void pru_phy_decoding(rtdm_timer_t *timer){
//	read_idx = readw(pru0+read_idx_storage_offset);
//	write_idx = readw(pru0+write_idx_storage_offset);
//
//	if(read_idx == write_idx){
//		return;
//	}
//
//	datalen = readw(pru0+read_idx);
//
//	if(datalen > 0 && datalen<= MTU_VALUE){
//
//		memset(rx_data,0,sizeof(uint8_t) * RX_DATA_LEN);
//
//		datalen += (OTHER_PACKET_CONTENT_BYTE + sizeof(uint16_t));
//		memcpy_fromio(rx_data, pru0+read_idx,datalen);
//		read_idx += datalen;
//
//		rx_pkt->datalen = datalen;
//		memcpy(rx_pkt->data,rx_data, rx_pkt->datalen);
//		mac_rx(vlc_devs,rx_pkt);
//		if(read_idx >= BUFFER_LIMIT){
//			read_idx = 4;
//		}
//		writew(read_idx,pru0+read_idx_storage_offset);
//	}
//}

void init_rs_error_packet(){
	int vlc_q_idx = 0;
	uint16_t packet_length = 74;
	uint16_t node_id = 98;
	uint16_t my_id = 1;
	unsigned int seq_number = 100;
	u8 vlc_q_payload = 0xCA;
	int i;

	rs_error_pkt->next = NULL;

	memcpy(&rs_error_pkt->data[vlc_q_idx],(u8*)&packet_length,sizeof(uint16_t));
	vlc_q_idx+=sizeof(uint16_t);
	memcpy(&rs_error_pkt->data[vlc_q_idx],(u8*)&node_id,sizeof(uint16_t));
	vlc_q_idx+=sizeof(uint16_t);
	memcpy(&rs_error_pkt->data[vlc_q_idx],(u8*)&my_id,sizeof(uint16_t));
	vlc_q_idx+=sizeof(uint16_t);
	memcpy(&rs_error_pkt->data[vlc_q_idx],(u8*)&seq_number,sizeof(unsigned int));
	vlc_q_idx+=sizeof(unsigned int);
//	get_random_bytes (&random_temp, sizeof (random_temp));
	//random_temp = 0xca;
	for(i=0;i<packet_length;i++){
		rs_error_pkt->data[vlc_q_idx++] = (u8) vlc_q_payload & 0xff;
		msleep(1);
	}

	rs_error_pkt->datalen = vlc_q_idx;
}

int init_module(void)
{
	printk(KERN_INFO "Hello world 1.\n");

	/*
	 * A non 0 return means init_module failed; module can't be loaded.
	 */
	//int ret = -ENOMEM;
	int ret = 0;


	vlc_devs = alloc_netdev(sizeof(struct vlc_priv), "vlc%d", vlc_init);
	if (vlc_devs == NULL){
		printk("Failed to allocate Net Dev\n");
		goto out;
	}

	ret = register_netdev(vlc_devs);
	if (ret)
		printk("VLC: error registering device \"%s\"\n", vlc_devs->name);

	vlc_dir = proc_mkdir("vlc", NULL);
	rx_device = create_proc_entry("rx", 0666, vlc_dir);
	tx_device = create_proc_entry("tx", 0666, vlc_dir);
	if (rx_device && tx_device) {
		printk("Proc entry is created\n");
		rx_device->data = &rx_device_value;
		rx_device->read_proc = proc_read;
		rx_device->write_proc = proc_write;

		tx_device->data = &tx_device_value;
		tx_device->read_proc = proc_read;
		tx_device->write_proc = proc_write;
	}

	pru0 = ioremap(PRU_ADDR,25000);

	if(!pru0)
		goto out;

	rs_decoder = init_rs(ecc_symsize, ecc_poly, 0, 1, ECC_LEN); // 0---FCR
	if (!rs_decoder) {
		printk(KERN_ERR "Could not create a RS decoder\n");
		ret = -ENOMEM;
		goto out;
	}

	init_rs_error_packet();

	task_phy_decoding = kthread_run(pru_phy_decoding,"RX thread","VLC_DECODING");
	if (IS_ERR(task_phy_decoding)){
		printk("Unable to start kernel threads. \n");
		ret= PTR_ERR(task_phy_decoding);
		task_phy_decoding = NULL;
		goto out;
	}

	//slot_ns = 1000000000 / frq;
//	slot_ns = 100000000;
//
//	printk("Slot in nanosecond: %d\n", slot_ns);
//	ret = rtdm_timer_init(&phy_timer, pru_phy_decoding, "phy timer");
//
//	if(ret) {
//	  rtdm_printk("PWM: error initializing up-timer: %i\n", ret);
//	  return ret;
//	}
//
//	ret = rtdm_timer_start(&phy_timer, slot_ns, slot_ns,
//				RTDM_TIMERMODE_RELATIVE);
//	if(ret) {
//		rtdm_printk("PWM: error starting up-timer: %i\n", ret);
//		return ret;
//	}


	out:
	printk("------EXIT vlc_init_module------\n");
	if (ret)
		cleanup_module();

	return ret;
}

void cleanup_module(void)
{
	printk(KERN_INFO "Goodbye world 1.\n");
//	rtdm_timer_destroy(&phy_timer);

	if (vlc_devs) {

//		printk("clean the pool\n");
		vlc_teardown_pool(vlc_devs);
		kfree(rx_pkt);
		kfree(rs_error_pkt);
//		kfree(rx_pkt_check_dup);

		printk("unregister the devs\n");
		unregister_netdev(vlc_devs);

		printk("free the devs\n");
		free_netdev(vlc_devs);
	}

	remove_proc_entry("rx", vlc_dir);
	remove_proc_entry("tx", vlc_dir);
	remove_proc_entry("vlc", NULL);

	iounmap(pru0);

	if (rs_decoder) {
		free_rs(rs_decoder);
	}

//	rtdm_timer_destroy(&phy_timer);

	if (task_phy_decoding) {
		kthread_stop(task_phy_decoding);
		task_phy_decoding = NULL;
	}
}
