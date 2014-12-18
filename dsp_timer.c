#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#include "wrap.h"
#include "mgr.h"
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
} while (0)

typedef void       (*TIMERFUNC)(union sigval);


#define SHOWAVT() do{printf("%s , %s , line : %d" , __FILE__ , __func__ , __LINE__); show_avtimers();\
}while(0);


struct timer_mem_t
{
		timer_t			timerid;
		struct sigevent sev;
		unsigned int	dspip;
		int				clifd;//cmd line 
		unsigned short	recvport;
};


static pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
static struct timer_mem_t * dsp_timer[128] = {0};

static int dsptimer_store(struct timer_mem_t ** p , unsigned int dspip)
{
		int index = dspip & 127; //dsp % 64

		pthread_mutex_lock(&timer_lock);
		*p = (struct timer_mem_t *)Malloc(sizeof(struct timer_mem_t));
		memset(*p , 0 , sizeof(struct timer_mem_t));

		if(dsp_timer[index] != NULL)
		{
				pthread_mutex_unlock(&timer_lock);
				printf("store , (dsp_timer[index] != NULL) !! \n");
				//exit(-1);
		}

		dsp_timer[index] = *p;
		pthread_mutex_unlock(&timer_lock);
}

static int dsptimer_release(unsigned int dspip)
{
		int index = dspip & 127; //dsp % 64

		pthread_mutex_lock(&timer_lock);
		if(dsp_timer[index] != NULL)
		{
				timer_delete(dsp_timer[index]->timerid);				
				free(dsp_timer[index]);
				dsp_timer[index] = NULL;
				pthread_mutex_unlock(&timer_lock);
				return 0;
		}
		pthread_mutex_unlock(&timer_lock);
		printf("dsptimer_release : avtimer[index] == NULL\n");
//		exit(-1);
		return -1;

}

//timer datastruct 4 confirm dspip isexist and store timer resource
static timer_t dsptimer_find(unsigned int dspip)
{
		int index = dspip & 127; //dsp % 64
		if(dsp_timer[index] != NULL)
			//fprintf(stderr ,"-----index : %d  , tid : %d \n",index , dsp_timer[index]->timerid);
		return (dsp_timer[index] == NULL ? NULL : dsp_timer[index]->timerid);
}




//////////////////////////////////////////////////////////////////////////////////////////////

static void* handler(void * p)
{
		static int i = 0;
		struct timer_mem_t * pt = (struct timer_mem_t*)p;

		//backdsp(pt->accept_avfd);
		//restart_av();
		//cli("video both stop");
		fprintf(stderr , "----- timeout %#x\n",pt->dspip);

		if(-1 == dspmgr_delete_dsp(pt->dspip))
			;//find avfd , send updata dsp pkt 4 av
		dsptimer_release(pt->dspip);
}

static int starttimer(int clifd , unsigned int dspip , long long timeout)
{
		struct timer_mem_t * p;
		struct itimerspec its;

		dsptimer_store(&p , dspip);

		p->dspip = dspip;
		p->clifd = clifd;

		p->sev.sigev_notify = SIGEV_THREAD;
		p->sev.sigev_value.sival_ptr = p;
		p->sev.sigev_notify_function = (TIMERFUNC)handler;
		if (timer_create(CLOCK_REALTIME, &(p->sev), &(p->timerid)) == -1)
				errExit("timer_create");
		//printf("timer ID is 0x%lx\n", (long)p->timerid);


		/* Start the timer */
		its.it_value.tv_sec = timeout;
		its.it_value.tv_nsec =0; 
		its.it_interval.tv_sec = timeout; 
		its.it_interval.tv_nsec = 0;

		if (timer_settime(p->timerid, 0, &its, NULL) == -1)
				errExit("timer_settime");

		return 0;
}

int dspmgr_reset_dsptimer(int clifd , unsigned int dspip  , long long timeout)
{
		struct itimerspec its;
		timer_t timerid;

		if(NULL != (timerid = dsptimer_find(dspip)))//timer datastruct 4 confirm dspip isexist and store timer resource
		{
				its.it_value.tv_sec = timeout;
				its.it_value.tv_nsec =0; 
				its.it_interval.tv_sec = timeout; 
				its.it_interval.tv_nsec = 0;

				if (timer_settime(timerid, 0, &its, NULL) == -1)
						err_sys("timer_settime");	
				return 0;
		}


		if(!starttimer(clifd , dspip , timeout))
				return 1;
}

int dspmgr_send_dsptimers(int fd , void * peeraddr , int addrlen)
{
		int i;
		char  sendbuf[256];
		Sendto(fd , "----------DSP-TIMER---------- :\n" , 32 , 0 , (struct sockaddr*)peeraddr , addrlen);

		for(i = 0 ; i < 128 ; i++)
		{
				if(dsp_timer[i] != 0)
				{
						sprintf(sendbuf , "keeper : fd %d , dspip 0x%.8x \n",i , dsp_timer[i]);
						Sendto(fd , sendbuf , strlen(sendbuf)  , 0 ,(struct sockaddr*)peeraddr , addrlen);
				}
		}
		return 0;
}


