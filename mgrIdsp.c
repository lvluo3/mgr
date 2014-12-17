#include <string.h>
#include <signal.h>


#include "wrap.h"
#include "mgr.h"

#define	MAXLINE		4096	/* max text line length */
#define SERV_PORT	1900


static int sockfd;
////////////////////////////////////////////////////////////////////////////
static int init_sock(int * fd)
{
		struct sockaddr_in  servaddr;//, cliaddr;

		sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family      = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port        = htons(SERV_PORT);

		Bind(sockfd, (SA *) &servaddr, sizeof(servaddr));
		*fd = sockfd;
}

static void dg_echo(int sockfd, SA *pcliaddr)//, socklen_t clilen)
{
		socklen_t	len;
		char		mesg[MAXLINE];
		int			recvlen;

		for ( ; ; ) 
		{
				len = sizeof(struct sockaddr);//clilen;
				recvlen = Recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
				if(recvlen >=  0)//recvlen >= 9 &&	0 == strncmp(mesg , "keepalive" , 9))
				{	
						int testdspip = ntohl( ((struct sockaddr_in *)pcliaddr)->sin_addr.s_addr );
						dspmgr_reset_dsptimer(0xffff , testdspip ,3 );	
						//fprintf(stderr,"recvlen :%d , peerip = %#x , mesg = %d \n" , recvlen , testdspip , ntohl(*((int*)mesg)));
						//Sendto(sockfd , mesg , recvlen , 0 , pcliaddr , len);

				}
		}
}

void * dspmgr_dspsvr(void * p)
{
		int fd;
		struct sockaddr	cliaddr ;

		init_sock(&fd);

		memset(&cliaddr , 0 , sizeof(struct sockaddr_in));
		dg_echo(fd , &cliaddr);
}


#define MGRINI  "mgr.ini"
#define IPSIZE  16
static int except(int * array , char * ex)
{
	int i ; 
	int j = 0;
	int k = 0;
	char tmp[16] = {0};
	int len;
	len = strlen(ex);

	for(i=0;i<len;i++)
	{	
		if(';' == ex[i])
		{
			tmp[j]=0;
			j = 0;
			array[k]=atoi(tmp) + 0xc0a8c800;
			k++;

			continue;
		}
		tmp[j] = ex[i];
		j++;

	}
	return k;//elements index + 1 = k
	
}

int dspmgr_read_ini()
{
		int ret1,ret2;
		unsigned int i , j , count;
		char beginstr[IPSIZE] , endstr[IPSIZE];
		unsigned int begin , end;
		char section[] ="dsp_section - 0";

		char exstr[128];
		int  array[16];
		int  arrlen;
		int  k = 0;
		char printstr[512];
		int x=0;
		count = read_profile_int("dspmgr_section_count","count",0,MGRINI);

		for(j = 0 ; j < count ; j++)
		{
				ret1 = read_profile_string(section,"begin",beginstr,IPSIZE,"",MGRINI);
				ret2 = read_profile_string(section ,"end", endstr ,IPSIZE, "" , MGRINI);
				
				read_profile_string(section ,"except", exstr ,128, "" , MGRINI);
				arrlen = except(array , exstr);
				/*
				for(x=0 ;x<arrlen ;x++ )
				{
					printf("---------- array[%d] has 0x%x\n", x , array[x]);
					
				}
*/
				begin = htonl(inet_addr(beginstr));
				end = htonl(inet_addr(endstr));

				sprintf(printstr ,"section : %s ,begin : %s %#x  , end :%s %#x\n" ,section , beginstr  , begin , endstr , end);
				printf(printstr);

				if(count > 9)
				{
						printf("count > 9\n");
						return -5;
				}
				if(0 == ret1 || 0 == ret2)
				{
						printf("invalid section , or no begin or no end .\n");
						return -4;
				}


				if(begin > end || (end - begin) > DSP_TOTAL)
				{
						printf("(begin > end || (end - begin) > DSP_TOTAL)\n");
						return -3;
				}

				if(begin == -1 || end == -1 )
				{
						printf("invalid ip \n");
						return -2;
				}

				for(i = begin ;i <= end ; i++)
				{
						if(k < arrlen && array[k] == i)
						{
							printf("array[] 0x%x\n",array[k]);
							k++;
							continue;
						}
						dspmgr_back_dsp(i);

						//dspmgr_addip4send_dspkeepalive(i);
						//dspmgr_reset_dsptimer(0xffff , i ,5 );	
				}

				section[14]++;
		}

		return (0 < count) ? 0 : -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////


struct node_t
{
		unsigned int dspip;
		struct node_t * next;
};

static struct node_t * head = NULL;
static struct node_t * tail = NULL;

int dspmgr_addip4send_dspkeepalive(unsigned int dspip)
{
		struct node_t * p = NULL;
		p = (struct node_t *)Malloc(sizeof(struct node_t));

		p->dspip = dspip;
		p->next = NULL;
		if(NULL == head)
		{
				head = p;	
				tail = p;
				return 1;
		}

		tail->next  = p;
		tail = p;
		return 0;
}

#pragma pack(1)
struct keepalive_pkt_t
{
	short len;
	short type;
	short sequ;
};

void * dspmgr_send_dspkeepalive(void * p)
{
		struct keepalive_pkt_t kp = {0,0,0};
		struct sockaddr_in  dspaddr;//, cliaddr;

		bzero(&dspaddr, sizeof(dspaddr));
		dspaddr.sin_family      = AF_INET;
		dspaddr.sin_port        = htons(SERV_PORT);
		while(1)
		{
				struct node_t * p = head;
				while(p != NULL)
				{
						dspaddr.sin_addr.s_addr = htonl(p->dspip);
						Sendto(sockfd , &kp , 6 , 0 ,(struct sockaddr*)&dspaddr , sizeof(dspaddr));
						p = p->next;
				}
				sleep(1);
		}
}



int dspmgr_send_dspkeepalive_ip(int fd , void * peeraddr , int addrlen)
{
		char  sendbuf[256];
	//	struct sockaddr_in  localshowaddr;//, cliaddr;
		struct node_t * p = head;
		int count = 0;
		Sendto(fd , "----------KEEPALIVE-DSP---------- :\n" , 36 , 0 , (struct sockaddr*)peeraddr , addrlen);
		while(p != NULL)
		{

				//printf("count %d , p : %#x , dspip : %#x , next : %#x .\n", p , p->dspip , p->next);
				sprintf(sendbuf , "count %d , p : %#x , dspip : %#x , next : %#x .\n",++count , p , p->dspip , p->next);
				Sendto(fd , sendbuf , strlen(sendbuf) , 0 ,(struct sockaddr*)peeraddr , addrlen);
				p = p->next;
		}
		return 0;
}
