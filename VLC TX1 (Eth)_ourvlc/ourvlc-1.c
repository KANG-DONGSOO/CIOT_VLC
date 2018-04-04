/*
 *  ourvlc-1.c - The simplest kernel module.
 */
#include "ourvlc-1.h"

MODULE_AUTHOR("Yogi Salomo");
MODULE_LICENSE("Dual BSD/GPL");

struct net_device *vlc_devs;


struct vlc_packet *tx_pkt;
struct vlc_packet *vlc_q_pkt;

static int mtu = 100;

static int flag_exit = 0;

static int flag_lock = 0;
module_param(flag_lock, int, 0);

volatile void* pru0;

struct proc_dir_entry *vlc_dir, *tx_device;
static int tx_device_value = 0;

uint32_t slot_ns;

static rtdm_timer_t phy_timer;

static int block_size = 200;

uint16_t read_idx = 4, write_idx =4;
int read_idx_offset = 2, write_idx_offset = 0;
int is_write_next = 1;


/* the Reed Solomon control structure */
static struct rs_control *rs_decoder;
uint16_t par[ECC_LEN];
int ecc_symsize = 8;
int ecc_poly = 0x11d;

static int curr_seq_number = 0;

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

struct vlc_packet *vlc_get_tx_buffer(struct net_device *dev)
{
	struct vlc_priv *priv = netdev_priv(dev);
	//unsigned long flags;
	struct vlc_packet *pkt;
    if (flag_lock)
        spin_lock_bh(&priv->lock);
	pkt = priv->ppool;
	priv->ppool = pkt->next;
	if (priv->ppool == NULL) {
		//printk (KERN_INFO "The MAC layer queue is full!\n");
		netif_stop_queue(dev);
	}
    if (flag_lock)
        spin_unlock_bh(&priv->lock);

	return pkt;
}

void vlc_release_buffer(struct vlc_packet *pkt)
{
	//unsigned long flags;
	struct vlc_priv *priv = netdev_priv(pkt->dev);

	if (flag_lock)
        spin_lock_bh(&priv->lock);
	pkt->next = priv->ppool;
	priv->ppool = pkt;
	if (flag_lock)
        spin_unlock_bh(&priv->lock);
	if (netif_queue_stopped(pkt->dev) && pkt->next == NULL && flag_exit == 0)
		netif_wake_queue(pkt->dev);
}

void vlc_enqueue_pkt(struct net_device *dev, struct vlc_packet *pkt)
{
    //unsigned long flags;
	struct vlc_priv *priv = netdev_priv(dev);
    struct vlc_packet *last_pkt;

    //spin_lock_bh(&priv->lock, flags);
	//pkt->next = priv->tx_queue;  /* FIXME - misorders packets */
	//priv->tx_queue = pkt;
    //spin_unlock_bh(&priv->lock, flags);

    /// Fix the misorder packets
    if (flag_lock)
        spin_lock_bh(&priv->lock);
    last_pkt = priv->tx_queue;
    if (last_pkt == NULL) {
        priv->tx_queue = pkt; // Enqueue the new packet
    } else {
        while (last_pkt != NULL && last_pkt->next != NULL) {
            last_pkt = last_pkt->next;
        }
        last_pkt->next = pkt; // Put new the pkt to the end of the queue
    }
    if (flag_lock)
        spin_unlock_bh(&priv->lock);
    ///
}

int vlc_tx(struct sk_buff *skb, struct net_device *dev)
{
	struct vlc_packet *tmp_pkt;

	dev->trans_start = jiffies;
	tmp_pkt = vlc_get_tx_buffer(dev);
	tmp_pkt->next = NULL; ///*******
	tmp_pkt->datalen = skb->len;
	memcpy(tmp_pkt->data, skb->data, skb->len);
	vlc_enqueue_pkt(dev, tmp_pkt);
//	printk("Enqueue a packet!\n");

	dev_kfree_skb(skb);
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

	tx_pkt = kmalloc (sizeof (struct vlc_packet), GFP_KERNEL);
	vlc_q_pkt = kmalloc(sizeof(struct vlc_packet), GFP_KERNEL);

	vlc_setup_pool(dev);
	priv->tx_queue = NULL;
	flag_exit = 0;
}

struct vlc_packet *vlc_dequeue_pkt(struct net_device *dev)
{
    //unsigned long flags;
    struct vlc_priv *priv = netdev_priv(dev);
	struct vlc_packet *pkt;

    if (flag_lock)
        spin_lock_bh(&priv->lock);
	pkt = priv->tx_queue;
	if (pkt != NULL)
		priv->tx_queue = pkt->next;
	if (flag_lock)
        spin_unlock_bh(&priv->lock);

    return pkt;
}

uint16_t write_packet_to_pru_memory(struct vlc_packet *pkt,uint16_t  write_idx){
	uint16_t curr_idx = write_idx;
	uint16_t packet_length;
	int datalength_idx = 0;
	int index_block, num_of_blocks = 0;
	int i;

	memcpy((u8*)&packet_length,&pkt->data[datalength_idx],sizeof(uint16_t));
	packet_length += (OTHER_PACKET_CONTENT_BYTE + sizeof(uint16_t));

	if (packet_length % block_size)
		num_of_blocks = packet_length / block_size + 1;
	else
		num_of_blocks = packet_length / block_size;

	for (index_block = 0; index_block < num_of_blocks; index_block++) {
		memset(par,0,sizeof(uint16_t)*ECC_LEN);
		if (index_block < num_of_blocks-1) {
			encode_rs8(rs_decoder,
					pkt->data+index_block*block_size,
					block_size, par, 0);
		} else {
			encode_rs8(rs_decoder,
					pkt->data+index_block*block_size,
					packet_length%block_size, par, 0);
		}
		for (i = 0; i < ECC_LEN; i++)
			pkt->data[packet_length+(index_block*ECC_LEN)+i] = par[i];
	}

	packet_length+= ECC_LEN;

	memcpy_toio(pru0+curr_idx,pkt->data+datalength_idx,packet_length);
	curr_idx += packet_length;


//	if(printk_ratelimit()){
//		printk("Curr IDX: %d",curr_idx);
//	}
	return curr_idx;
}

//static int mac_tx(void *data)
void phy_timer_handler(rtdm_timer_t *timer)
{
	struct vlc_priv *priv = netdev_priv(vlc_devs);

	if(is_write_next){
		write_idx = readw(pru0+write_idx_offset);
		if(priv->tx_queue){
			tx_pkt = vlc_dequeue_pkt(vlc_devs);
			write_idx = write_packet_to_pru_memory(tx_pkt,write_idx);
			vlc_release_buffer(tx_pkt);

//			if(write_idx >= BUFFER_LIMIT){
//				write_idx = 4;
//			}
//
//			is_write_next = 0;
		}
		else{
//			memcpy(&vlc_q_pkt->data[6],(u8*)&curr_seq_number,sizeof(unsigned int));
//			curr_seq_number = (curr_seq_number + 1) % 500000;
			write_idx =  write_packet_to_pru_memory(vlc_q_pkt,write_idx);
		}
		if(write_idx >= BUFFER_LIMIT){
			write_idx = 4;
		}

		is_write_next = 0;
	}

	read_idx = readw(pru0+read_idx_offset);
	if(read_idx != write_idx){
		writew(write_idx,pru0+write_idx_offset);
		is_write_next = 1;
	}
}

void init_vlc_quality_packet(){
	int vlc_q_idx = 0;
	uint16_t packet_length = PAYLOAD_LEN;
	uint16_t node_id = DUMMY_PACKET_NODE_ID;
	uint16_t my_id = VLC_TX_ID;
	unsigned int seq_number = 100;
	u8 vlc_q_payload = 0xCA;
	int i;

	vlc_q_pkt->next = NULL;

	memcpy(&vlc_q_pkt->data[vlc_q_idx],(u8*)&packet_length,sizeof(uint16_t));
	vlc_q_idx+=sizeof(uint16_t);
	memcpy(&vlc_q_pkt->data[vlc_q_idx],(u8*)&node_id,sizeof(uint16_t));
	vlc_q_idx+=sizeof(uint16_t);
	memcpy(&vlc_q_pkt->data[vlc_q_idx],(u8*)&my_id,sizeof(uint16_t));
	vlc_q_idx+=sizeof(uint16_t);
	memcpy(&vlc_q_pkt->data[vlc_q_idx],(u8*)&seq_number,sizeof(unsigned int));
	vlc_q_idx+=sizeof(unsigned int);
//	get_random_bytes (&random_temp, sizeof (random_temp));
	//random_temp = 0xca;
	for(i=0;i<packet_length;i++){
		vlc_q_pkt->data[vlc_q_idx++] = (u8) vlc_q_payload & 0xff;
		msleep(1);
	}

	vlc_q_pkt->datalen = vlc_q_idx;
}

int init_module(void)
{
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
	tx_device = create_proc_entry("tx", 0666, vlc_dir);
	if (tx_device) {
		printk("Proc entry is created\n");
		tx_device->data = &tx_device_value;
		tx_device->read_proc = proc_read;
		tx_device->write_proc = proc_write;
	}

	pru0 = ioremap(PRU_ADDR,25000);
	if(!pru0)
		goto out;

	uint32_t initial_write_read_idx = 0x00040004;
	writel(initial_write_read_idx,pru0);
	init_vlc_quality_packet();

	rs_decoder = init_rs(ecc_symsize, ecc_poly, 0, 1, ECC_LEN); // 0---FCR
	if (!rs_decoder) {
		printk(KERN_ERR "Could not create a RS decoder\n");
		ret = -ENOMEM;
		goto out;
	}

//	slot_ns = 1000000000 / frq;
	slot_ns = 125000;
//	printk("Slot in nanosecond: %d\n", slot_ns);
	ret = rtdm_timer_init(&phy_timer, phy_timer_handler, "phy timer");

	if(ret) {
	  rtdm_printk("PWM: error initializing up-timer: %i\n", ret);
	  return ret;
	}

	ret = rtdm_timer_start(&phy_timer, slot_ns, slot_ns,
	            RTDM_TIMERMODE_RELATIVE);
	if(ret) {
		rtdm_printk("PWM: error starting up-timer: %i\n", ret);
		return ret;
	}
	rtdm_printk("PWM: timers created\n");

	out:
	printk("------EXIT vlc_init_module------\n");
	if (ret)
		cleanup_module();

	return ret;
}

void cleanup_module(void)
{
	struct vlc_packet *pkt;
	struct vlc_priv *priv = netdev_priv(vlc_devs);

	if (vlc_devs) {

		printk("clean the pool\n");
		while(priv->tx_queue) {
			pkt = vlc_dequeue_pkt(vlc_devs);
			vlc_release_buffer(pkt);
		}
		vlc_teardown_pool(vlc_devs);
		kfree(tx_pkt);
		kfree(vlc_q_pkt);

		printk("unregister the devs\n");
		unregister_netdev(vlc_devs);

		printk("free the devs\n");
		free_netdev(vlc_devs);
	}

	 // Free the reed solomon resources
	if (rs_decoder) {
		free_rs(rs_decoder);
	}

	rtdm_timer_destroy(&phy_timer);

	remove_proc_entry("tx", vlc_dir);
	remove_proc_entry("vlc", NULL);

	iounmap(pru0);
}
