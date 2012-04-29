#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <errno.h>

static int init_hotplug_sock(void)
{
    struct sockaddr_nl snl;
    const int buffersize = 16 * 1024 * 1024;
    int retval;
    
    memset(&snl, 0x00, sizeof(struct sockaddr_nl));
    
    snl.nl_family = AF_NETLINK;
    snl.nl_pid = getpid();  //self pid
    snl.nl_groups = 1;      //standard output as multicast
   
    //int socket(int domain, int type, int protocol); 
    int hotplug_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (hotplug_sock == -1) {
        printf("error getting socket: %s", strerror(errno));
        return -1;
    }
    
    printf("test errno: %s\n", strerror(errno));
    
    /* set receive buffersize 
     * int		(*setsockopt)	(struct sock *sk, int level, int optname, char __user *optval, unsigned int optlen);  */
    setsockopt(hotplug_sock, SOL_SOCKET, SO_RCVBUFFORCE, &buffersize, sizeof(buffersize));
    
    //int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    //sockfd is achieved from socket()
    retval = bind(hotplug_sock, (struct sockaddr *) &snl, sizeof(struct sockaddr_nl));
    
    if (retval < 0) {
        printf("bind failed: %s", strerror(errno));
        close(hotplug_sock);
        hotplug_sock = -1;
        return -1;
    }
    
    return hotplug_sock;
}

#define UEVENT_BUFFER_SIZE      2048

int main(int argc, char* argv[])
{
    int hotplug_sock = init_hotplug_sock();
    while(1)
    {
        char buf[UEVENT_BUFFER_SIZE*2] = {0};
        //ssize_t recv(int socket, void *buffer, size_t length, int flags);
        recv(hotplug_sock, &buf, sizeof(buf), 0); //config snl(sockaddr) -> socket -> setsockopt -> bind -> recv(connection-mode)
        printf("%s\n", buf);
    }
    
    return 0;
}
