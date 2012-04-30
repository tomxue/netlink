#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#define NETLINK_USER 31     //the user defined channel, the key factor

struct sock *nl_sk = NULL;

static void hello_nl_recv_msg(struct sk_buff *skb) 
{
    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    char *msg="Hello from kernel";
    int res;

    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    msg_size=strlen(msg);
    //for receiving...
    nlh=(struct nlmsghdr*)skb->data;    //nlh message comes from skb's data... (sk_buff: unsigned char *data)
    /*  static inline void *nlmsg_data(const struct nlmsghdr *nlh)
        {
	        return (unsigned char *) nlh + NLMSG_HDRLEN;
        } 
    nlmsg_data - head of message payload */
    printk(KERN_INFO "Netlink received msg payload: %s\n",(char*)nlmsg_data(nlh));
    
    //for sending...
    pid = nlh->nlmsg_pid; // Sending process port ID, will send new message back to the 'user space sender'
    
    skb_out = nlmsg_new(msg_size,0);    //nlmsg_new - Allocate a new netlink message: skb_out

    if(!skb_out)
    {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }

    nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);   //nlmsg_put - Add a new netlink message to an skb
                                                        //argument 1 - @skb: socket buffer to store message in
                                                        //argument 2 - @pid = 0, from kernel
                                                        //NLMSG_DONE - End of a dump
                                                        //nlh points to skb_out now...

    //#define NETLINK_CB(skb)		(*(struct netlink_skb_parms*)&((skb)->cb))
    //cb: This is the control buffer. It is free to use for every layer. Please put your private variables there
    /* struct netlink_skb_parms {
	struct ucred		creds;		// Skb credentials
	__u32			pid;
	__u32			dst_group;
    }; */
    //map skb->cb (char	cb[48] __aligned(8); control buffer) to "struct netlink_skb_parms", so it has field pid/dst_group
    //so there should be convention: cb[48] is divided into creds/pid/dst_group...to convey those info
    NETLINK_CB(skb_out).dst_group = 0;                  /* not in mcast group */
    strncpy(nlmsg_data(nlh),msg,msg_size);              //char *strncpy(char *dest, const char *src, size_t count)
                                                        //msg "Hello from kernel" => nlh -> skb_out

    res=nlmsg_unicast(nl_sk,skb_out,pid);               //nlmsg_unicast - unicast a netlink message
                                                        //@pid: netlink pid of the destination socket
    if(res<0)
        printk(KERN_INFO "Error while sending bak to user\n");
}

static int __init hello_init(void) 
{
    printk("Entering: %s\n",__FUNCTION__);
    //struct net init_net; defined in net_namespace.c
    /* netlink_kernel_create(struct net *net, int unit, unsigned int groups, 
                             void (*input)(struct sk_buff *skb),
	                         struct mutex *cb_mutex, struct module *module) */
    //unit=NETLINK_USER: refer to some kernel examples
    //groups = 0, unicast
    //nl_sk: global sock, will be sent to hello_nl_recv_msg as argument (nl_sk ->...-> skb) and return from below func, by Tom Xue, not totally verified
    nl_sk=netlink_kernel_create(&init_net, NETLINK_USER, 0, hello_nl_recv_msg, NULL, THIS_MODULE);
    if(!nl_sk)
    {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }

    return 0;
}

static void __exit hello_exit(void) 
{
    printk(KERN_INFO "exiting hello module\n");
    netlink_kernel_release(nl_sk);
}

module_init(hello_init); 
module_exit(hello_exit);
MODULE_LICENSE("GPL");
