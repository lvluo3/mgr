#include <string.h>
#include "wrap.h"
#include "mgr.h"


int begine_avfd = -1;


unsigned int dspip_keeper[128] = {0};

static int av_keep_dspip(int avfd , unsigned int dspip )
{
		int index = avfd - begine_avfd;

		if(index >= 128 || index <= 0)
				return -1;
		if(dspip_keeper[index] != 0)
				return -2;

		dspip_keeper[index] = dspip ;
		printf("avfd %d, index %d, dspip 0x%x\n",avfd , index , dspip);
		return 0;
}
#include <pthread.h>
static pthread_mutex_t avfdkeeper_lock = PTHREAD_MUTEX_INITIALIZER;
unsigned int dspmgr_av_discard_dspip(int avfd)
{
		unsigned int dspip;
		int index = avfd - begine_avfd;
		if(index >= 128 || index <= 0)
				return -1;
		if(dspip_keeper[index] != 0)
		{
				dspip = dspip_keeper[index];

				pthread_mutex_lock(&avfdkeeper_lock);
				dspip_keeper[index] = 0;
				pthread_mutex_unlock(&avfdkeeper_lock);
				return dspip;
		}
		return -1;
}

static unsigned int av_find_dspip(int avfd)
{
		int index = avfd - begine_avfd;
		if(avfd >= begine_avfd)
		{
				if(dspip_keeper[index] == 0)
						return 0;
				return dspip_keeper[index];
		}
		return 0;
}

int dsp_isinkeep(unsigned int dspip)
{
	int i;
	for(i=0 ; i < 128 ; i++ )
	{
		if (dspip_keeper[i] == dspip)
		{
			//printf("dsp is in keeper\n");
			return 0;
		}
	}
	return -1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
#define MAX_EPOLL_EVENTS 64
static int efd;

static int srvsock_init(unsigned short port)
{
		int                 listenfd;
		int		    reuseopt = 1;
		struct sockaddr_in  servaddr;

		listenfd = Socket(AF_INET, SOCK_STREAM, 0);


		memset(&servaddr , 0 , sizeof(servaddr));
		servaddr.sin_family      = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port        = htons(port);

		Setsockopt(listenfd ,SOL_SOCKET , SO_REUSEADDR , &reuseopt ,  sizeof(int));
		Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

		Listen(listenfd,5);

		return listenfd;
}

static int incoming_connections(int listen_fd )
{
		struct sockaddr_in		in_addr;  
		socklen_t			in_len;  
		int					fd;  
		struct epoll_event	event;
		//char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];  
		struct timespec time;
		socklen_t       opt_len = 0;

		in_len = sizeof in_addr;  
		fd = Accept(listen_fd, (struct sockaddr * )&in_addr, &in_len);

		//printf("dspmgr		:		peer connection listenfd : %d\n" ,listen_fd);
		printf("---------------connect come , avfd : %d , peerip %#x , peerport %d\n ",fd , ntohl(in_addr.sin_addr.s_addr) , ntohs(in_addr.sin_port));


		time.tv_sec = 0;
		time.tv_nsec = 500000;
		opt_len = sizeof(struct timespec);

		Setsockopt(fd ,SOL_SOCKET , SO_RCVTIMEO , &time ,  opt_len);

		begine_avfd > fd ? begine_avfd = fd : begine_avfd;	
		PNB("dspmgr		:		begineavfd : %d , acceptfd : %d " , begine_avfd , fd);

		event.data.fd = fd;  
		event.events = EPOLLIN;  
		Epoll_ctl (efd, EPOLL_CTL_ADD, fd, &event);
}

#define PKT_GET_LEN		10000
#define PKT_KA_LEN		4
#define PKT_HEAD_LEN	2

enum pkt_type_t
{
		KEEPALIVE,
		REQUEST_DSP,
		MODIFY_DSP
};

enum dsp_type_t
{
		H264 = 1,
		H2642HPVC,
		HPVC2H264
};

struct recvclt_pkt_t
{
		short	pkttype;
		short	dsptype;//or count for keepalive
};

struct response_pkt_t
{
		short		pkttype;
		short		reserve;
		unsigned int	dspip;
};

static void peer_arouse_err(int fd)
{
		unsigned int dspip;
		static int count =0;
		// Will closing a file descriptor cause it to be removed from all epoll sets automatically , EEpoll_ctl (efd, EPOLL_CTL_DEL, fd, NULL);
		Epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		dspip = dspmgr_av_discard_dspip(fd);
		if(dspip == -1)
		{
				printf("????????????????????????????????????????????????????????? fd %d can not find in keeper\n",fd);
				return;
		}
		//dspmgr_cli_stop(dspip);
		dspmgr_back_dsp(dspip);
		//dspmgr_release_avtimer(fd);
		printf("------------------------------------------------------------count %d ,back avfd : %d , dsp : 0x%x \n",++count , fd,dspip);		
		return;
}

static int EWrite(int fd, void *ptr, size_t nbytes)
{
		if (write(fd, ptr, nbytes) != nbytes)//-1 syserr , other codeerr
		{
				PNB("dspmgr		:		write err len :%d ",nbytes);
				peer_arouse_err(fd);
				return -1;
		}
		return 0;
}

void dspmgr_send_keepalive(int fd)
{
		short msglen;
		char  buf[128];

		msglen = htons(8);
		if(-1 == EWrite(fd , &msglen , PKT_HEAD_LEN))
				return;
		((struct response_pkt_t*)buf)->pkttype  = htons(KEEPALIVE);
		((struct response_pkt_t*)buf)->reserve  = htons(0);
		((struct response_pkt_t*)buf)->dspip    = htonl(0);
		if(-1 == EWrite(fd , buf , sizeof(struct response_pkt_t)))
				return;
		/*	printf("-----ewrit keepalive-----\n"
			"msglen		: 0x%.4x\n"
			"buf->pkttype	: 0x%.4x\n"
			"buf->reserve	: 0x%.4x\n"
			"buf->dspip	: 0x%.8x\n",
			msglen,((struct response_pkt_t*)buf)->pkttype,
			((struct response_pkt_t*)buf)->reserve,((struct response_pkt_t*)buf)->dspip);
			*/
}

#include "mgrIhpvc.h"

extern int hpvcfd;
extern struct  hpvc_usr_t hpvc_usr;

static int process_pkt(int fd , char * buf)
{
		char sendbuf[512] = {0};
		short pkttype = ntohs(*((short*)buf));
		short dsptype = ntohs(*((short*)(buf+2)));
		PNB("dspmgr		:		pkttype %d , dsptype %d " , pkttype , dsptype);
		switch(pkttype)
		{
				short msglen;
				int	  xxx;
				unsigned int dspip;
				case KEEPALIVE:
				{
						//dspmgre_get_peer_keepalive(fd);
						break;
				}
				case REQUEST_DSP: 
				{
						if(dsptype == H264)
						{
								//get x264 dsp
								dspip = av_find_dspip(fd);
								if(0 == dspip)
								{
										int ret;
										ret = dspmgr_get_dsp(&dspip);
										if(ret != 0)//no dsp can use
										{
												msglen = htons(8);
												if(-1 == EWrite(fd , &msglen , PKT_HEAD_LEN))
														return -1;
												((struct response_pkt_t*)buf)->pkttype	= htons(REQUEST_DSP);
												((struct response_pkt_t*)buf)->reserve	= htons(0);
												((struct response_pkt_t*)buf)->dspip	= htonl(0);
												if(-1 == EWrite(fd , buf , sizeof(struct response_pkt_t)))
														return -1;

												printf("-----ewrit h264 empty -----\n"
																"msglen		: 0x %.4x\n"
																"buf->pkttype	: 0x%.4x\n"
																"buf->reserve	: 0x%.4x\n"
																"buf->dspip	: 0x%.8x\n",
																msglen,((struct response_pkt_t*)buf)->pkttype,
																((struct response_pkt_t*)buf)->reserve,((struct response_pkt_t*)buf)->dspip);
												return -2;
										}

										av_keep_dspip(fd , dspip);
										//dspmgr_reset_avtimer(fd,3);
								}


								msglen = htons(8);
								if(-1 == EWrite(fd , &msglen , PKT_HEAD_LEN))
										return -1;
								((struct response_pkt_t*)buf)->pkttype	= htons(REQUEST_DSP);
								((struct response_pkt_t*)buf)->reserve	= htons(0);
								((struct response_pkt_t*)buf)->dspip	= htonl(dspip);
								if(-1 == EWrite(fd , buf , sizeof(struct response_pkt_t)))
										return -1;

								printf("------------------------------------------------------------ get avfd %d , dspip 0x%x\n",fd,dspip);

								/*
								   printf("-----ewrit h264 dspip-----\n"
								   "msglen		: 0x%.4x\n"
								   "buf->pkttype	: 0x%.4x\n"
								   "buf->reserve	: 0x%.4x\n"
								   "buf->dspip	: 0x%.8x\n",
								   msglen,((struct response_pkt_t*)buf)->pkttype,
								   ((struct response_pkt_t*)buf)->reserve,((struct response_pkt_t*)buf)->dspip);
								   */
								return 0;

						}
						else if(dsptype == H2642HPVC || dsptype == HPVC2H264)
						{
								if(0 < hpvcfd)
								{
										//get hpvc dsp
										hpvc_usr.p = &fd;
										hpvcmgr_enfifo(&hpvc_usr);

										msglen = htons(4);
										if(-1 == EWrite(hpvcfd , &msglen , PKT_HEAD_LEN))
												return -1; 
										if(-1 == EWrite(hpvcfd , buf , 4))
												return -1;
										PNB("dspmgr		:		send to hpvcmgr : pkttype %d , dsptype %d " , ntohs(*((short*)buf)) , ntohs(*((short*)(buf+2))));
								}
								else
								{
										msglen = htons(8);
										if(-1 == EWrite(fd , &msglen , PKT_HEAD_LEN))
												return -1;
										((struct response_pkt_t*)buf)->pkttype	= htons(REQUEST_DSP);
										((struct response_pkt_t*)buf)->reserve	= htons(0);
										((struct response_pkt_t*)buf)->dspip	= htonl(0xffffffff);
										if(-1 == EWrite(fd , buf , sizeof(struct response_pkt_t)))
												return -1;

										printf("-----ewrit hpvc empty -----\n"
														"msglen		: 0x %.4x\n"
														"buf->pkttype	: 0x%.4x\n"
														"buf->reserve	: 0x%.4x\n"
														"buf->dspip	: 0x%.8x\n",
														msglen,((struct response_pkt_t*)buf)->pkttype,
														((struct response_pkt_t*)buf)->reserve,((struct response_pkt_t*)buf)->dspip);
										return -2;
								}
						}
						else
								return -1;

						break;
				}
				default:
				{
						//Epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
						//close(fd);
						return -2;
				}
		}
		return 0;
}

static int recv_data(int fd)
{
		int	ret = 0;                                                                 
		char	buf[512];

		ret = Recv(fd , buf , PKT_HEAD_LEN , MSG_WAITALL);
		//printf("dspmgr		:		recv pkt head len : %d \n",ret);
		if(ret <= 0)
		{
				//	printf("dspmgr		:		recv head <= 0 ,ret : %d, fd : %d\n" ,ret ,  fd);
				peer_arouse_err(fd);
				return -1;
		}
		else
		{
				short msglen = ntohs(*((short*)buf));

				if(msglen <= 0)//for balm-av send x.25 pkt
				{
						return -1;
				}

				ret = Recv(fd , buf , msglen , MSG_WAITALL);
				//printf("dspmgr		:		recv pkt body len = %d , msglen : %d \n",ret , msglen);
				if(ret <= 0)
				{
						printf("dspmgr		:		recv body <= 0 ,ret : %d , fd : %d\n" ,ret , fd);
						peer_arouse_err(fd);
						return -1;
				}

				process_pkt(fd , buf);
		}

		return 0;
}

static void handle_sigpipe(int x)
{
		//	printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
		return;
}


int dspmgr_avsvr()
{
		int listenfd;
		struct epoll_event listen_event;
		struct epoll_event events[MAX_EPOLL_EVENTS * sizeof(listen_event)];

		Signal(SIGPIPE , handle_sigpipe);

		listenfd = srvsock_init(1998);

		efd = Epoll_create();

		listen_event.data.fd = listenfd;  
		listen_event.events = EPOLLIN;
		Epoll_ctl(efd , EPOLL_CTL_ADD , listenfd , &listen_event);


		while(1)
		{
				int n , i ;
				n = epoll_wait(efd , events , MAX_EPOLL_EVENTS , -1);
				if(n == -1)
				{
						PNBSQ("dspmgr		:		epoll_wait");
				}

				for(i = 0 ; i < n ; i++)
				{
						if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))  
						{  
								/* An error has occured on this fd, or the socket is not 
								   ready for reading (why were we notified then?) */  
								fprintf (stderr, "dspmgr		:		epoll error, fd : %d\n", events[i].data.fd);  
								peer_arouse_err(events[i].data.fd);
								continue;  
						} 
						else if(listenfd == events[i].data.fd)
						{
								//incoming connections
								//printf("dspmgr		:		incoming_connections( listenfd : %d );epoll n = %d \n ",listenfd,n);
								incoming_connections(listenfd);
						}
						else if((events[i].events & EPOLLIN))
						{
								//recv data
								//printf("dspmgr		:		recv_data( recvfd : %d );\n",events[i].data.fd);
								recv_data(events[i].data.fd);
						}
						else
						{
								printf("dspmgr		:		else events[i].events = 0x%x\n" , events[i].events);
						}

				}

		}

		close (listenfd);  

		return 0;	
}
