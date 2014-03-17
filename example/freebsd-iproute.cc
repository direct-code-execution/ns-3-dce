#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

struct sockaddr_in_f {
  unsigned char sin_len;
  unsigned char sin_family;
  unsigned short sin_port;
  struct in_addr sin_addr;
  char	sin_zero[8];
};
struct sockaddr_f {
  unsigned char sa_len;		/* total length */
  unsigned char sa_family;	/* address family */
  char sa_data[14];	/* actually longer; address value */
};
#define		IFNAMSIZ	16
struct ifaliasreq {
  char	ifra_name[IFNAMSIZ];		/* if name, e.g. "en0" */
  struct	sockaddr_f ifra_addr;
  struct	sockaddr_f ifra_broadaddr;
  struct	sockaddr_f ifra_mask;
  int	ifra_vhid;
};

#undef _IOC
#undef _IOW
#undef IOC_IN
#undef IOC_OUT

#define	IOCPARM_SHIFT	13		/* number of bits for ioctl size */
#define	IOCPARM_MASK	((1 << IOCPARM_SHIFT) - 1) /* parameter length mask */
#define	IOCPARM_MAX	(1 << IOCPARM_SHIFT) /* max size of ioctl */
#define	IOC_VOID	0x20000000	/* no parameters */
#define	IOC_OUT		0x40000000	/* copy out parameters */
#define	IOC_IN		0x80000000	/* copy in parameters */

#define	_IOC(inout,group,num,len)	((unsigned long) \
	((inout) | (((len) & IOCPARM_MASK) << 16) | ((group) << 8) | (num)))
#define	_IOW(g,n,t)	_IOC(IOC_IN,	(g), (n), sizeof(t))
#define	SIOCAIFADDR	 _IOW('i', 43, struct ifaliasreq)/* add/chg IF alias */

int 
main (int argc, char *argv[])
{
  struct ifaliasreq ifra;
  struct sockaddr_in_f *sinp;
  int s;

  s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s < 0) {
    perror ("ifconfig_inet: socket(PF_INET, SOCK_RAW, 0)");
    return (-1);
  }

  memset (&ifra, 0, sizeof(ifra));
  strcpy (ifra.ifra_name, argv[1]);

  sinp = (struct sockaddr_in_f *)&ifra.ifra_addr;
  sinp->sin_family = AF_INET;
  sinp->sin_len = sizeof(ifra.ifra_addr);
  sinp->sin_addr.s_addr = inet_addr(argv[2]);

  sinp = (struct sockaddr_in_f *)&ifra.ifra_mask;
  sinp->sin_family = AF_INET;
  sinp->sin_len = sizeof(ifra.ifra_addr);
  sinp->sin_addr.s_addr = inet_addr(argv[3]);

  if (ioctl(s, SIOCAIFADDR, &ifra) < 0) {
    perror("ifconfig_inet: ioctl(SIOCAIFADDR)");
    close(s);
    return (-1);
  }
  
  close(s);
  return 0;
}
