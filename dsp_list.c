#include <stdio.h>
#include <pthread.h>

#include "mgr.h"

struct node_t
{
		unsigned int dspip;
		struct node_t * next;
};

struct list_t
{
		struct node_t * head;
		struct node_t * tail;
		int		size;
};

static struct node_t mem[DSP_TOTAL] = {0};
static struct list_t mem_list = {0};

void dspmgr_mem_init()
{
		int i ;
		for(i = 0 ; i < DSP_TOTAL ; i++)
		{
				mem[i].next = &mem[i+1];
		}

		mem[DSP_TOTAL -1].next = NULL;
		mem_list.head = &mem[0];
		mem_list.tail = &mem[DSP_TOTAL-1];
		mem_list.size = DSP_TOTAL;
}

static struct node_t * mem_malloc()
{
		struct node_t * p = mem_list.head;
		if(mem_list.size <= 0)
		{
				return NULL;
		}

		mem_list.head = mem_list.head->next;
		mem_list.size--;

		return p;
}

static void mem_free(struct node_t * p)
{
		if(mem_list.size <= 0)
		{
				mem_list.tail = p;
				mem_list.head = p;
		}
		else
		{
				mem_list.tail->next = p;
				mem_list.tail = p;
		}
		mem_list.size++;
}



///////////////////////////////////////////////////////

#define SHOW_LIST() do{ \
		fprintf(stderr , "-------------%s,line : %d--------------------\n",__func__ , __LINE__); \
		show_list(); \
}while(0);




static pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
static struct list_t list = {0};

static void show_list()
{

		struct node_t * p = list.head;
		while(NULL != p)
		{
				fprintf(stderr , "p : 0x%.8x , dspip : 0x%.8x , next : 0x%.8x \n", p , p->dspip , p->next);
				p = p->next;
		}
}

int dspmgr_back_dsp(unsigned int dspip)
{
		pthread_mutex_lock(&list_lock);
		struct node_t * p = mem_malloc();
		if(p == NULL)
		{
				PNBQ("anomalous  mem_empty!! list.size : %d" , list.size);
				pthread_mutex_unlock(&list_lock);
				return -1;
		}	

		p->dspip = dspip;
		p->next = NULL;

		if(list.size <= 0)
		{
				list.head = p;
				list.tail = p;	
		}
		else
		{
				list.tail->next = p;
				list.tail = p;	
		}
		list.size++;
		pthread_mutex_unlock(&list_lock);
		//SHOW_LIST();	
		return 0;
}

int dspmgr_get_dsp(unsigned int * pdspip)
{
		pthread_mutex_lock(&list_lock);
		struct node_t * p = list.head;
		if(list.size <= 0)
		{
				printf("anomalous : %s , list.size : %d\n",__func__ , list.size);
				pthread_mutex_unlock(&list_lock);
				return -1;
		}	

		*pdspip = p->dspip;

		list.head = p->next;
		p->dspip = 0;
		p->next  = NULL;
		mem_free(p);
		list.size--;
		pthread_mutex_unlock(&list_lock);
		//SHOW_LIST();


		return 0;
}


/*can't find specify dsp return -1 , otherwise return 0 */
int dspmgr_delete_dsp(unsigned int dspip)
{
		struct node_t * pp;
		struct node_t * p;

		pthread_mutex_lock(&list_lock);
		pp = list.head;
		p = list.head;


		do{
				if(p->dspip == dspip)
				{
						(pp != list.head) ? (pp->next = p->next) : (list.head = p->next);
						(NULL != p->next) ? (p->next = NULL) : (list.tail = pp);
						p->dspip = 0;
						mem_free(p);
						list.size--;

						pthread_mutex_unlock(&list_lock);
						return 0;
				}

				pp = p;
				p = p->next;
		}while(NULL != p);

		pthread_mutex_unlock(&list_lock);
		return -1;
}
/*is dspip exist return 0 , otherwise return -1*/
int dspmgr_isexist_dsp(unsigned int dspip)
{
		struct node_t * p ;
		p = list.head;

		while(NULL != p)
		{
			if(p->dspip == dspip)
				return 0;
			p = p->next;
		}
		return -1;
}


////////////////////////////////////////////////////////////////////////////////////////show thread
#include <string.h>
#include "wrap.h"

#define SERV_PORT	1998

enum 
{
		SHOW_MEM = '0',
		SHOW_LIST,
		SHOW_MEM_LIST,
		SHOW_INUSE,
		SHOW_KEEPALIVE_DSP,
		SHOW_DSPTIMER,
		SHOW_HPVCMEM,
		SHOW_HPVCLIST
};

extern unsigned int dspip_keeper[128];//inuse dsp info only read !!!! 

static int send_keeper(int fd , char * str , struct sockaddr * ppeeraddr , socklen_t addrlen)
{
		int i ;
		Sendto(fd , "----------KEEPER---------- :\n"  , 29 , 0 , ppeeraddr , addrlen);
		for(i = 0 ; i < 128 ; i++)
		{
				if(dspip_keeper[i] != 0)
				{
						sprintf(str , "keeper : fd %d , dspip 0x%.8x \n",i , dspip_keeper[i]);
						Sendto(fd , str , strlen(str)  , 0 ,ppeeraddr , addrlen);
				}
		}
		return 0;
}

static int init_sock(int * fd)
{
		int                 sockfd;
		struct sockaddr_in  servaddr;//, cliaddr;

		sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

		memset(&servaddr , 0 , sizeof(servaddr));
		servaddr.sin_family      = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port        = htons(SERV_PORT);

		Bind(sockfd, (SA *) &servaddr, sizeof(servaddr));
		*fd = sockfd;
}

static int send_mem(int fd , char * str , struct sockaddr * ppeeraddr , socklen_t addrlen)
{
		int i ;
		Sendto(fd , "----------MEM---------- :\n"  , 26 , 0 , ppeeraddr , addrlen);
		for(i = 0 ; i < DSP_TOTAL ; i++)
		{
				sprintf(str , "p : 0x%.8x , dspip : 0x%.8x , next : 0x%.8x \n", &mem[i] , mem[i].dspip , mem[i].next);
				Sendto(fd , str , strlen(str)  , 0 ,ppeeraddr , addrlen);
		}
		return 0;
}

static int send_mem_list(int fd , char * str ,  struct sockaddr * ppeeraddr , socklen_t addrlen)
{
		struct node_t * p = mem_list.head;

		Sendto(fd , "----------MEM_LIST---------- :\n" , 31 , 0 , ppeeraddr , addrlen);
		while(NULL != p)
		{
				sprintf(str , "p : 0x%.8x , dspip : 0x%.8x , next : 0x%.8x \n", p , p->dspip , p->next);
				Sendto(fd , str , strlen(str)  , 0 , ppeeraddr , addrlen);
				p = p->next;
		}
		return 0; 
}

static int send_list(int fd , char * str ,  struct sockaddr * ppeeraddr , socklen_t addrlen)
{
		int count = 0;
		struct node_t * p = list.head;

		Sendto(fd , "----------LIST---------- :\n" , 27 , 0 , ppeeraddr , addrlen);
		while(NULL != p)
		{
				sprintf(str , "count %d , p : 0x%.8x , dspip : 0x%.8x , next : 0x%.8x \n", ++count , p , p->dspip , p->next);
				Sendto(fd , str , strlen(str)  , 0 , ppeeraddr , addrlen);
				p = p->next;
		}
		return 0; 
}



void * dspmgr_show_list(void * p)
{
		int fd;
		ssize_t recvlen = 0 ;
		socklen_t addrlen = 0 ;

		char buf[512];
		struct sockaddr peeraddr;

		init_sock(&fd);	

		while(1)
		{
				memset(&peeraddr , 0 , sizeof(peeraddr));
				recvlen = Recvfrom(fd, buf , 256 , 0, &peeraddr , &addrlen);
				if(recvlen >=  0 )//recvlen >= 9 &&	0 == strncmp(mesg , "keepalive" , 9))
				{	
						unsigned int ip = htonl( ((struct sockaddr_in *)&peeraddr)->sin_addr.s_addr );
						unsigned short port = htons( ((struct sockaddr_in *)&peeraddr)->sin_port );
						//fprintf(stderr,"peeraddr ip  = 0x%x , port = %d , *buf : %c  , mem : %c , list %c\n" , ip , port , *buf , SHOW_MEM , SHOW_LIST);
						if(ip != 0 && port != 0)
						{
								switch(*buf)
								{
										case  SHOW_LIST:

												send_list(fd , buf , &peeraddr , addrlen);
												break;

										case  SHOW_MEM:

												send_mem(fd , buf , &peeraddr , addrlen);
												break;

										case  SHOW_MEM_LIST:

												send_mem_list(fd , buf , &peeraddr , addrlen);
												break;

										case SHOW_INUSE:

												send_keeper(fd , buf , &peeraddr , addrlen);
												break;

										case SHOW_KEEPALIVE_DSP:
												
												dspmgr_send_dspkeepalive_ip(fd , &peeraddr , addrlen);
												break;

										case SHOW_DSPTIMER:
												
												dspmgr_send_dsptimers(fd , &peeraddr , addrlen);
												break;

										case SHOW_HPVCLIST:

												send_hpvcmem(fd , buf , &peeraddr , addrlen);	
												break;

										case SHOW_HPVCMEM:

												send_hpvclist(fd , buf , &peeraddr , addrlen);	
												break;
										default :

												//Sendto(fd , "param err\n" , 10  , 0 , &peeraddr , addrlen);
												break;


								}
						}
				}

		}
		close(fd);
}
