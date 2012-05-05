/*
 * =====================================================================================
 *
 *       Filename:  gpio_interrupt.c
 *
 *    Description:  basic interrupt trial program
 *
 *        Version:  1.0
 *        Created:  11/27/11 10:04:25
 *       Revision:  none
 *       Compiler:  gcc
 *       Platform:  BB-xM-RevC, Ubuntu 3.0.4-x3
 *
 *         Author:  Tom Xue (), tom.xue@nokia.com
 *        Company:  Nokia
 *
 * =====================================================================================
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/interrupt.h> //request_irq 
#include <linux/gpio.h>	//OMAP_GPIO_IRQ
#include <plat/mux.h>	//omap_cfg_reg
#include <linux/irq.h>	//IRQ_TYPE_LEVEL_LOW

#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>


#define NETLINK_USER 31     //the user defined channel, the key factor

struct sock *nl_sk = NULL;

MODULE_LICENSE("GPL");

//MMC2_DAT6, P9-pin5, system default GPIO input with pullup
#define OMAP3_GPIO138          (138)
int irq;
struct nlmsghdr *nlh;
int pid;
struct sk_buff *skb_out;
int msg_size;
char *msg="shutdown";
int res;

static void hello_nl_recv_msg(struct sk_buff *skb) 
{
   
    printk(KERN_ALERT "Entering: %s\n", __FUNCTION__);

    msg_size=strlen(msg);
    //for receiving...
    nlh=(struct nlmsghdr*)skb->data;    //nlh message comes from skb's data... (sk_buff: unsigned char *data)
    /*  static inline void *nlmsg_data(const struct nlmsghdr *nlh)
        {
	        return (unsigned char *) nlh + NLMSG_HDRLEN;
        } 
    nlmsg_data - head of message payload */
    printk(KERN_ALERT "Netlink received msg payload: %s\n",(char*)nlmsg_data(nlh));
    
}

static irqreturn_t my_interrupt(){
	printk(KERN_ALERT "my_interrupt executed!\n");

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

static int hello_init(void) {
	int ret;

	//below function: Sets the Omap MUX and PULL_DWN registers based on the table and judge 'cpu_class_is_omap1'
	//omap_cfg_reg(OMAP3_GPIO138);
		
	ret = gpio_request(OMAP3_GPIO138, "OMAP3_GPIO138");
	if(ret < 0)
		printk(KERN_ALERT "gpio_request failed!\n");
		
	gpio_direction_input(OMAP3_GPIO138);	
	
	irq = OMAP_GPIO_IRQ(OMAP3_GPIO138);	//irq33 <-> GPIO module 5: includes gpio_138
	printk(KERN_ALERT "OMAP_GPIO_IRQ success! The irq = %d\n", irq);
	
	irq_set_irq_type(irq, IRQ_TYPE_EDGE_FALLING);
	enable_irq(gpio_to_irq(OMAP3_GPIO138));
	
	ret = request_irq(irq, my_interrupt, IRQF_DISABLED, "my_interrupt_proc", NULL);
	if (ret==0)
        printk(KERN_ALERT "request_irq success!\n");
    else
        printk(KERN_ALERT "request_irq fail!\n"); 
		
    printk(KERN_ALERT "Hello, Tom Xue! From inside kernel driver!\n");

    //below is for netlink operation...
    printk(KERN_ALERT "Entering: %s\n",__FUNCTION__);
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
    else
        printk(KERN_ALERT "Creating netlink successfully!\n");

    return 0;
}

static void hello_exit(void)
{
    disable_irq(irq);
    free_irq(irq, NULL);
    gpio_free(OMAP3_GPIO138);
    printk(KERN_INFO "Goodbye, Tom Xue! From inside kernel driver!\n");

    printk(KERN_INFO "exiting hello module\n");
    netlink_kernel_release(nl_sk);
}

module_init(hello_init);
module_exit(hello_exit);
