#include <string.h>
#include <time.h>
#include "wrap.h"
#include "mgr.h"

#if 0
struct cli_timer_mem_t
{
	timer_t		timerid;
	struct sigevent sev;
	int		stopfd;
};
#endif

typedef void       (*TIMERFUNC)(union sigval);


static struct sigevent sev;
static timer_t         timerid;

static void* handler(void * p)
{
	//static int i = 0;
	//struct cli_timer_mem_t * pt = (struct cli_timer_mem_t*)p;

	//close(pt->stopfd);

	//timer_delete(pt->timerid);				
	//free(pt);
	printf("thread xxxxxxxxxxxxxxxxx \n");
}

void alrm_handler(int i)
{
	return ;
}


static int starttimer(int arm_second)
{
	//struct cli_timer_mem_t * p;
	struct itimerspec its;


	//p = (struct cli_timer_mem_t *)Malloc(sizeof(struct cli_timer_mem_t));
	//memset(p , 0 , sizeof(struct cli_timer_mem_t));

	//p->stopfd = stopfd;
	
	Signal(SIGALRM , alrm_handler);

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGALRM;
	//	sev.sigev_value.sival_ptr = p;
	//	sev.sigev_notify_function = (TIMERFUNC)handler;

	if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
		PNBSQ("timer_create");
	//printf("timer ID is 0x%lx\n", (long)p->timerid);


	/* Start the timer */
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = arm_second; 
	its.it_interval.tv_sec = 0; 
	its.it_interval.tv_nsec = 0;


	if (timer_settime(timerid, 0, &its, NULL) == -1)
		PNBSQ("timer_settime");

	return 0;
}

static int recv_rn(int fd , char * buf , int len , unsigned int dspip)
{
	int	ret;
	char    *p = buf;
	while(1)
	{
		ret = recv(fd , p  , 512 , 0);
		if(ret <= 0)
		{	
			close(fd);
			//err_sys_exit();
		}

		if(!memcmp(p , "\r\n" , 2))
			break;
		p = p + ret;
	}
	printf("peerdsp 0x%.8x :\n%s\n" , dspip , buf);
	return ret;
}

int dspmgr_cli_stop(unsigned int dspip)
{
	int    fd;
	struct sockaddr_in  connaddr;
	char	buf[512];
	char 	*p = buf;
	int	ret;
	struct timespec time;
	socklen_t	opt_len = 0;

	fd = Socket(AF_INET, SOCK_STREAM, 0);

	time.tv_sec = 0;
	time.tv_nsec = 500000;
	opt_len = sizeof(struct timespec);

	Setsockopt(fd ,SOL_SOCKET , SO_RCVTIMEO , &time ,  opt_len);

	memset(&connaddr , 0 , sizeof(connaddr));
	connaddr.sin_family      = AF_INET;
	connaddr.sin_addr.s_addr = htonl(dspip);
	connaddr.sin_port        = htons(1998);

	starttimer(50);

	ret = connect(fd , (SA*)&connaddr , sizeof(connaddr));
	if(ret == -1)
	{
		PNBS("");
		close(fd);
		timer_delete(timerid);				
		return -1;
		//err_sys_exit();
	}

	starttimer(0);


	recv_rn(fd , buf , 512 , dspip);

	Send(fd , "video cvt stop\r\n" , 16 , 0);

	recv_rn(fd , buf ,512 , dspip);




	close(fd);
	timer_delete(timerid);
	return 0;
}

