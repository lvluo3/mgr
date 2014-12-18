#define DSP_TOTAL 16

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define err_sys_exit() {fprintf(stderr , "%s , %s ,line : %d",__FILE__ , __func__ , __LINE__);  perror("");  exit(-1);}
#include <stdlib.h>
#define PNBQ(fmt , ...)  do{ \
	err_quitx( __FILE__, __LINE__, __func__, fmt , ## __VA_ARGS__); \
	exit(-1); \
}while(0);

#define PNB(fmt , ...)  do{ \
	err_quitx( __FILE__, __LINE__, __func__, fmt , ## __VA_ARGS__); \
}while(0);

#define PNBS(fmt , ...)  do{ \
	err_quitx( __FILE__, __LINE__, __func__, fmt , ## __VA_ARGS__); \
	perror(""); \
}while(0);

#define PNBSQ(fmt , ...)  do{ \
	err_quitx( __FILE__, __LINE__, __func__, fmt , ## __VA_ARGS__); \
	perror(""); \
	exit(-1); \
}while(0);


void  err_quitx(const char* file , int line , const char * func ,const char *fmt, ...);

int dspmgr_reset_avtimer(int accept_avfd , long long timeout);
int dspmgr_release_avtimer(int avfd);

int dspmgr_reset_dsptimer(int clifd , unsigned int dspip  , long long timeout);

//param all host byte order
void dspmgr_mem_init();
int dspmgr_get_dsp(unsigned int * pdspip);
int dspmgr_back_dsp(unsigned int dspip);

int dspmgr_isexist_dsp(unsigned int dspip);
int dspmgr_delete_dsp(unsigned int dspip);



int dspmgr_read_ini();
void * dspmgr_dspsvr(void * p);
int dspmgr_addip4send_dspkeepalive(unsigned int dspip);
//void * dspmgr_send_dspkeepalive(void * p);
int dspmgr_send_dspkeepalive_ip(int fd , void * peeraddr , int addrlen);

int dspmgr_avsvr();

int dspmgr_cli_stop(unsigned int dspip);

unsigned int dspmgr_av_discard_dspip(int avfd);

void * dspmgr_show_list(void * p);

void dspmgr_send_keepalive(int fd);
void dspmgre_get_peer_keepalive(int fd);
