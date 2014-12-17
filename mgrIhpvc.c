#include <string.h>
#include <pthread.h>
#include "wrap.h"
#include "mgrIhpvc.h"


/////////////////////////////////////////////////////////////////////
void hpvcmgr_mem_init(struct list_t * plist , int data_size)
{
		int i ;
		struct node_t * mem;

		mem = (struct node_t*)malloc(sizeof(struct node_t) * MEMSIZE);

		for(i = 0 ; i < MEMSIZE - 1 ; i++)
		{
				mem[i].data = malloc(data_size);
				mem[i].next = &mem[i + 1];
		}

		mem[MEMSIZE - 1].data = malloc(data_size);
		mem[MEMSIZE - 1].next = NULL;

		plist->head = &mem[0];
		plist->tail = &mem[MEMSIZE - 1];
		plist->size = MEMSIZE;

}

int hpvcmgr_destroy_memdata(struct list_t * mem_list)
{
		int i ;
		struct node_t * p = mem_list->head;
		while(p != NULL)
		{
				free(p->data);
				p = p->next;
		}
		return 0;
}

struct node_t * hpvcmgr_mem_malloc(struct list_t *  mem_list )
{
		struct node_t * p = mem_list->head;
		if(mem_list->size <= 0)
		{
				return NULL;
		}

		mem_list->head = mem_list->head->next;
		mem_list->size--;

		return p;
}

void hpvcmgr_mem_free(struct list_t * mem_list , struct node_t * p)
{
		if(mem_list->size <= 0)
		{
				mem_list->tail = p;
				mem_list->head = p;
		}
		else
		{
				mem_list->tail->next = p;
				mem_list->tail = p;
		}
		mem_list->size++;
}






///////////////////////////////////////////////////////////////////////////
static pthread_mutex_t fifo_lock = PTHREAD_MUTEX_INITIALIZER;



int hpvcmgr_enfifo(struct hpvc_usr_t * hpvc_usr)
{
		struct node_t * p;
		struct list_t * pfifo = &hpvc_usr->fifo;
		struct list_t * pmem_list = &hpvc_usr->mem_list;

		pthread_mutex_lock(&fifo_lock);
		p = hpvcmgr_mem_malloc(pmem_list);
		if(p == NULL)
		{
				pthread_mutex_unlock(&fifo_lock);
				return -1;
		}

		hpvc_usr->filldata(p->data);
		p->next = NULL;

		if(pfifo->size <= 0)
		{
				pfifo->head = p;
				pfifo->tail = p;
		}
		else
		{
				pfifo->tail->next = p;
		}
		pfifo->size++;
		pthread_mutex_unlock(&fifo_lock);
		return 0;
}

int hpvcmgr_defifo(struct hpvc_usr_t * hpvc_usr)
{

		struct node_t * p;
		struct list_t * pfifo = &hpvc_usr->fifo;
		struct list_t * pmem_list = &hpvc_usr->mem_list;

		pthread_mutex_lock(&fifo_lock);
		p = pfifo->head;
		if(pfifo->size <= 0)
		{
				pthread_mutex_unlock(&fifo_lock);
				return -1;
		}

		//	*pdspip = p->dspip;
		//	p->dspip = 0;

		hpvc_usr->emptydata(p->data);
		pfifo->head = p->next;

		p->next  = NULL;
		hpvcmgr_mem_free(pmem_list , p);
		pfifo->size--;
		pthread_mutex_unlock(&fifo_lock);


		return 0;
}

////////////////////////////////////////////////////////////////////
struct data_t
{
		//just int fd so no need data_t
};

int     hpvcfd;
struct  hpvc_usr_t hpvc_usr; //class  object

static int filldata(void * pdata)
{
		(*(int*)pdata) = (*(int*)hpvc_usr.p);
}

static int emptydata(void * pdata)
{
		(*(int*)hpvc_usr.p) = *((int *)pdata);
}

static void handle_sigpipe2(int x)
{
		return ;
}
int hpvc_client_init()
{
		struct sockaddr_in  servaddr;

		hpvc_usr.filldata = filldata;
		hpvc_usr.emptydata = emptydata;

		Signal(SIGPIPE , handle_sigpipe2);


		hpvcfd = Socket(AF_INET, SOCK_STREAM, 0);


		memset(&servaddr , 0 , sizeof(servaddr));
		servaddr.sin_family      = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr("192.168.200.70");//htonl(INADDR_ANY);
		servaddr.sin_port        = htons(2998);

		if(-1 == connect(hpvcfd , (SA *) &servaddr, sizeof(servaddr)))
		{
				//perror("conncet hpvcmgr error:");
				close(hpvcfd);
				hpvcfd = -1;
				return -1;
				//exit(-1);
		}
		hpvcmgr_mem_init(&hpvc_usr.mem_list , sizeof(int)); //data is fd

		return 0;
}

void * hpvc_client_thread(void * p)
{
		int avfd;
		char buf[512];
		int  len;
		while(1)
		{
				if( -1 != hpvc_client_init())
				{
						while(1)
						{
								len = Recv(hpvcfd , buf , 512 , 0);
								if(len == 0)
								{
										fprintf(stderr , "mgrIhpvc recv == 0\n");
										close(hpvcfd);
										hpvcfd = -1;
										break;
										//			exit(-1);
								}

								hpvcmgr_defifo(&hpvc_usr);
								avfd = *((int *)hpvc_usr.p);

								fprintf(stderr , "recv hpvcmgr reply , pktlen : %d , pkttype : %d , reserve %d , dspnum : %d , avfd : %d\n" , 
												htons(*((short*)buf)) , htons(*((short*)(buf + 2))) , htons(*(short*)(buf + 4)) ,  htonl(*(int*)(buf + 6)) ,  avfd);

								send(avfd , buf , len , 0);
						}
				}
				else
				{
						sleep(5);
						continue;
				}
				hpvcmgr_destroy_memdata(&hpvc_usr.mem_list);
				sleep(1);
		}
}
////////////////////////////////////////////////////////////////////////////////////////////

int send_hpvclist(int fd , char * str ,  struct sockaddr * ppeeraddr , socklen_t addrlen )
{
		struct node_t * p = hpvc_usr.fifo.head;

		Sendto(fd , "----------HPVCLIST---------- :\n" , 31 , 0 , ppeeraddr , addrlen);
		while(NULL != p)
		{
				sprintf(str , "p : 0x%.8x , avfd : 0x%.8x , next : 0x%.8x \n", p , *((int*)p->data) , p->next);
				Sendto(fd , str , strlen(str)  , 0 , ppeeraddr , addrlen);
				p = p->next;
		}
		return 0; 
}

int send_hpvcmem(int fd , char * str ,  struct sockaddr * ppeeraddr , socklen_t addrlen)
{
		struct node_t * p = hpvc_usr.mem_list.head;

		Sendto(fd , "----------HPVCMEM---------- :\n" , 30 , 0 , ppeeraddr , addrlen);
		while(NULL != p)
		{
				sprintf(str , "p : 0x%.8x , avfd : 0x%.8x , next : 0x%.8x \n", p , *((int*)p->data) , p->next);
				Sendto(fd , str , strlen(str)  , 0 , ppeeraddr , addrlen);
				p = p->next;
		}
		return 0; 
}
