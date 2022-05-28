#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/moduleparam.h>
#include <linux/in.h>
#include <net/arp.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h> 
#include <linux/uaccess.h> 
#include <linux/slab.h>
#include <linux/string.h>


#define MY_MODULE "vni: "
#define DBG(...) if( debug != 0 ) pr_info(MY_MODULE __VA_ARGS__ )
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0) 
	#define HAVE_PROC_OPS 
#endif 

#define BUF_SIZE 10000
#define PROC_FILE_NAME "vni0"

static struct proc_dir_entry *proc_file; 
static char *proc_msg;
static size_t proc_msg_len = 0;
	
static int debug = 0;
module_param(debug, int, 0);

static char *link = "lo";
module_param(link, charp, 0);

static char *ifname = "vni%d";

static int dest_port_filter = 12345;
module_param(dest_port_filter, int, 0);

static struct net_device_stats stats;

static struct net_device *child = NULL;
struct priv {
    struct net_device *parent;
};

static char *strIP( u32 addr ) {    
   static char saddr[MAX_ADDR_LEN];
   sprintf(saddr, "%d.%d.%d.%d",
			ntohl(addr) >> 24, (ntohl(addr) >> 16) & 0x00FF,
			(ntohl(addr) >> 8) & 0x0000FF, (ntohl(addr)) & 0x000000FF
          );
   return saddr;
}

static void write_proc_msg(char *saddr, int sport, char *daddr, int dport) {
	static char msg[MAX_ADDR_LEN * 2 + 20];
	size_t left;
	sprintf(msg, "from %s %d to %s %d\n", saddr, sport, daddr, dport);
	left = BUF_SIZE - proc_msg_len;
	if (left <= strlen(msg)) {
		pr_info(MY_MODULE "buffer ended\n");
		proc_msg_len = 0;
	}
	strcpy(proc_msg + proc_msg_len, msg);
	proc_msg_len += strlen(msg);
}


static char check_frame(struct sk_buff *skb) {
	struct iphdr *ip = (struct iphdr *)skb_network_header(skb);
	char *daddr, *saddr;
	struct udphdr *udp = NULL;
	int source_port, dest_port;
	
	//DBG("protocol %d\n", ntohs(skb->protocol));
	
	//if(skb->protocol != htons(ETH_P_IP))
		//return 0;
	
    ip = (struct iphdr *) skb_network_header(skb);
	daddr = strIP(ip->daddr);
	saddr = strIP(ip->saddr);
	
	DBG("check ip-packet from %s to %s by protocol %d\n", saddr, daddr, ip->protocol);
    
	if (IPPROTO_UDP == ip->protocol) {
		udp = udp_hdr(skb);
        source_port = ntohs(udp->source);
		dest_port = ntohs(udp->dest);
		if (dest_port != dest_port_filter) {
			return 0;
		}
		write_proc_msg(saddr, source_port, daddr, dest_port);
		pr_info(MY_MODULE "UDP datagram from %s %d to %s %d\n", saddr, source_port, daddr, dest_port);
        return 1;
    }
    return 0;
}

static rx_handler_result_t handle_frame(struct sk_buff **pskb) {    
	DBG("handle frame\n");
	if (!check_frame(*pskb)) {
		DBG("not suitable\n");
		return RX_HANDLER_PASS;
	}
	DBG("suitable, update rx stats: old = %ld, new = %ld\n", stats.rx_packets, stats.rx_packets + 1);
	stats.rx_packets++;
	stats.rx_bytes += (*pskb)->len;
	(*pskb)->dev = child;
	return RX_HANDLER_ANOTHER;
}

static int open(struct net_device *dev) {
    netif_start_queue(dev);
    pr_info(MY_MODULE "%s device opened\n", dev->name);
    return 0; 
} 

static int stop(struct net_device *dev) {
    netif_stop_queue(dev);
    pr_info(MY_MODULE "%s device closed\n", dev->name);
    return 0; 
} 

static netdev_tx_t start_xmit(struct sk_buff *skb, struct net_device *dev) {
    struct priv *priv = netdev_priv(dev);
	
	DBG("start xmit from %s\n", dev->name);
    if (check_frame(skb) && priv->parent) {
		DBG("suitable, update tx stats: old = %ld, new = %ld\n", stats.tx_packets, stats.tx_packets + 1);
		stats.tx_packets++;
		stats.tx_bytes += skb->len;
		
        skb->dev = priv->parent;
        skb->priority = 1;
        dev_queue_xmit(skb);
		DBG("redirect from %s to %s\n", dev->name, skb->dev->name);
    } else {
		DBG("not suitable");
	}
    return NETDEV_TX_OK;
}

static struct net_device_stats *get_stats(struct net_device *dev) {
    return &stats;
} 

static struct net_device_ops net_device_ops = {
    .ndo_open = open,
    .ndo_stop = stop,
    .ndo_get_stats = get_stats,
    .ndo_start_xmit = start_xmit
};

static void setup(struct net_device *dev) {
    ether_setup(dev);
    memset(netdev_priv(dev), 0, sizeof(struct priv));
    dev->netdev_ops = &net_device_ops;
} 

// Proc file

static ssize_t proc_file_read(struct file *f, char __user *buf, size_t len, loff_t *off) {
	if (*off >= proc_msg_len) {
		return 0;
	}
	
	len = min(proc_msg_len - (size_t) *off, len);
	if (copy_to_user(buf, proc_msg + (size_t) *(off), len)) {
		return -EFAULT;
	}
	
	*off += len; 
	return len; 
} 

#ifdef HAVE_PROC_OPS 
	static const struct proc_ops proc_fops = { 
		.proc_read = proc_file_read
	}; 
#else 
	static const struct file_operations proc_fops = {
		.read = proc_file_read
	};
#endif

void vni_exit(void) {
    struct priv *priv = netdev_priv(child);
    if (priv->parent) {
        rtnl_lock();
        netdev_rx_handler_unregister(priv->parent);
        rtnl_unlock();
        pr_info(MY_MODULE "%s unregister rx handler for %s\n", THIS_MODULE->name, priv->parent->name);
    }
    unregister_netdev(child);
    free_netdev(child);
	proc_remove(proc_file);
	kfree(proc_msg);
    pr_info(MY_MODULE "Module %s unloaded\n", THIS_MODULE->name); 
} 

int __init vni_init(void) {
	int err = 0;
	struct priv *priv;
	
	proc_file = proc_create(PROC_FILE_NAME, 0444, NULL, &proc_fops);
	if (!proc_file) {
		pr_err(MY_MODULE "proc file create error\n");
		return -1;
	}
	
    child = alloc_netdev(sizeof(struct priv), ifname, NET_NAME_UNKNOWN, setup);
    if (child == NULL) {
		proc_remove(proc_file);
        pr_err(MY_MODULE "%s allocate error\n", THIS_MODULE->name);
        return -ENOMEM;
    }
    priv = netdev_priv(child);
    priv->parent = __dev_get_by_name(&init_net, link); //parent interface
    if (!priv->parent) {
        pr_err(MY_MODULE "%s no such net: %s\n", THIS_MODULE->name, link);
		proc_remove(proc_file);
        free_netdev(child);
        return -ENODEV;
    }
    if (priv->parent->type != ARPHRD_ETHER && priv->parent->type != ARPHRD_LOOPBACK) {
        pr_err(MY_MODULE "%s illegal net type\n", THIS_MODULE->name); 
		proc_remove(proc_file);
        free_netdev(child);
        return -EINVAL;
    }

    //copy IP, MAC and other information
    memcpy(child->dev_addr, priv->parent->dev_addr, ETH_ALEN);
    memcpy(child->broadcast, priv->parent->broadcast, ETH_ALEN);
	err = dev_alloc_name(child, child->name);
    if (err) {
        pr_err(MY_MODULE "%s allocate name, error %i\n", THIS_MODULE->name, err);
		proc_remove(proc_file);
        free_netdev(child);
        return -EIO;
    }

    register_netdev(child);
    rtnl_lock();
    netdev_rx_handler_register(priv->parent, &handle_frame, NULL);
    rtnl_unlock();
	proc_msg = kmalloc(BUF_SIZE, GFP_KERNEL);
	memset(proc_msg, 0, BUF_SIZE);
	if (!proc_msg) {
		pr_err(MY_MODULE "couldn't allocate buffer\n");
		vni_exit();
		return -1;
	}	
    pr_info(MY_MODULE "Module %s loaded\n", THIS_MODULE->name);
    pr_info(MY_MODULE "%s create link %s\n", THIS_MODULE->name, child->name);
    pr_info(MY_MODULE "%s registered rx handler for %s\n", THIS_MODULE->name, priv->parent->name);
	pr_info(MY_MODULE "destination port filter %d\n", dest_port_filter);
    return 0; 
}

module_init(vni_init);
module_exit(vni_exit);

MODULE_AUTHOR("Filimonov Alexey");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Description");