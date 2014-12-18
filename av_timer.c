#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "wrap.h"
#include "mgr.h"
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
} while (0)

static pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t keepalive_count_lock = PTHREAD_MUTEX_INITIALIZER;




////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void       (*TIMERFUNC)(union sigval);

struct timer_mem_t
{
	timer_t			timerid;
	struct sigevent		sev;
	int			accept_avfd;
	int			send_count;
};

static struct timer_mem_t * avtimer[1024];
extern int begine_avfd;
static void show_avtimers();

#define SHOWAVT() do{printf("%s , %s , line : %d" , __FILE__ , __func__ , __LINE__); show_avtimers();\
}while(0);

static void store_avtimer(int avfd , struct timer_mem_t ** p )
{
	int index = avfd - begine_avfd;

	if(index >= 1024 || index <= 0)
		PNBQ("avtimer[%d] fd : %d" , index , avfd);

	if(avtimer[index] != 0)
		PNBQ("avtimer[%d] fd : %d" , index , avfd);

	*p = (struct timer_mem_t *)Malloc(sizeof(struct timer_mem_t));
	memset(*p , 0 , sizeof(struct timer_mem_t));
	avtimer[index] = *p;
	SHOWAVT();
}

int dspmgr_release_avtimer(int avfd)
{
	int index = avfd - begine_avfd;

	if(index >= 1024 || index <= 0)
		return -1;

	pthread_mutex_lock(&timer_lock);
	if(avtimer[index] != NULL)
	{
		timer_delete(avtimer[index]->timerid);				
		free(avtimer[index]);
		avtimer[index] = NULL;
		pthread_mutex_unlock(&timer_lock);
		return 0;
	}
	pthread_mutex_unlock(&timer_lock);
	return -1;
}

struct timer_mem_t * find_ptimemem(int avfd)
{
	int index = avfd - begine_avfd;

	//SHOWAVT();
	if(avfd >= begine_avfd)
	{
		if(avtimer[index] == NULL)
			return NULL;
		//				printf("index : %d , avfd : %d , begine_avfd : %d ", index , avfd , begine_avfd);
		return avtimer[index];
	}
	return NULL;
}


static void show_avtimers()
{
	int i;
	printf("-------------------%s----------------------\n",__func__);
	for(i = 0 ; i < 1024 ; i ++)
	{
		if(NULL == avtimer[i])
			return ;
		fprintf(stderr , "avfd : %d , tid : 0x%.8x , pmem : 0x%.8x\n",
				avtimer[i]->accept_avfd,
				avtimer[i]->timerid,
				avtimer[i]
		       );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////

void dspmgre_get_peer_keepalive(int fd)
{
	struct timer_mem_t * p;
	p = find_ptimemem(fd);
	if(p == NULL)
	{
		PNBQ("get_peer_keepalive buf can't find timer!!!");
	}

	pthread_mutex_lock(&keepalive_count_lock);
	p->send_count = 0;
	pthread_mutex_unlock(&keepalive_count_lock);

}

static void* handler(void * p)
{
	static int i = 0;
	struct timer_mem_t * pt = (struct timer_mem_t*)p;
	unsigned int dspip;


	if(pt->send_count >= 3)
	{
		close(pt->accept_avfd);
		dspip = dspmgr_av_discard_dspip(pt->accept_avfd);
		if(dspip != 0)
		{
			//dspmgr_cli_stop(dspip);
			dspmgr_back_dsp(dspip);
			dspmgr_release_avtimer(pt->accept_avfd);
		}
		return NULL;
	}

	dspmgr_send_keepalive(pt->accept_avfd);

	pthread_mutex_lock(&keepalive_count_lock);
	pt->send_count++;
	pthread_mutex_unlock(&keepalive_count_lock);
}


static int starttimer(long long timeout , int accept_avfd)
{
	struct timer_mem_t * p;
	struct itimerspec its;

	store_avtimer(accept_avfd, &p);

	p->send_count  = 0;
	p->accept_avfd = accept_avfd;

	p->sev.sigev_notify = SIGEV_THREAD;//SIGEV_SIGNAL;
	p->sev.sigev_value.sival_ptr = p;
	p->sev.sigev_notify_function = (TIMERFUNC)handler;
	if (timer_create(CLOCK_REALTIME, &(p->sev), &(p->timerid)) == -1)
		errExit("timer_create");
	//printf("timer ID is 0x%lx\n", (long)p->timerid);


	/* Start the timer */
	its.it_value.tv_sec = 0;//timeout;
	its.it_value.tv_nsec = 1; 
	its.it_interval.tv_sec = timeout; 
	its.it_interval.tv_nsec = 0;


	if (timer_settime(p->timerid, 0, &its, NULL) == -1)
		errExit("timer_settime");

	return 0;
}

int dspmgr_reset_avtimer(int accept_avfd , long long timeout)
{
	struct itimerspec its;
	timer_t timerid = NULL;
	struct timer_mem_t * p;


	p = find_ptimemem(accept_avfd);
	if(p == NULL)
	{
		if(!starttimer(timeout , accept_avfd))
			return 0;
	}
	//		PNB("reset timerid : %x" , timerid);
	its.it_value.tv_sec = timeout;
	its.it_value.tv_nsec =0; 
	its.it_interval.tv_sec = timeout; 
	its.it_interval.tv_nsec = 0;

	if (timer_settime(p->timerid, 0, &its, NULL) == -1)
		err_sys("timer_settime");	
	//	errExit("timer_settime");
	return 1;
}
















#if 0
/* Create the timer */
sev.sigev_signo = SIGUSR1;
sev.sigev_notify = SIGEV_SIGNAL;
sev.sigev_value.sival_ptr = &timerid;
if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
	errExit("timer_create");

	printf("timer ID is 0x%lx\n", (long) timerid);

	Signal(SIGUSR1 , handler1);


	//		memset(&its,0,sizeof(struct itimerspec));
	//		if (timer_settime(timerid, 0, &its, NULL) == -1)
	//                errExit("timer_settime");


	//		printf("-------------------%s----------------------\n",__func__);
	//		printf("----------timer_id = 0x%lx , i = %d \n" ,pt->timerid , i++ );

	//		timer_delete(pt->timerid);	
	//		free(pt);
#endif
