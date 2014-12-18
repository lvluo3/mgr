#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

#include "wrap.h"
#include "mgr.h"



void * hpvc_client_thread(void * p);
void * dspmgr_send_dspkeepalive(void * x);

int main(int argc , char * argv[])
{
		int ret;
		pthread_t tid , tid1 , tid2 , tid3;


		dspmgr_mem_init();

		if(0 > (ret = dspmgr_read_ini()))
		{
				printf("read_ini failed ! return %d . -1:dspmgr_section_count unmatch . \n" , ret);
				return -1;
		}
		
		//dsp interface 
		pthread_create(&tid2 , NULL , dspmgr_dspsvr , NULL);
		pthread_create(&tid3 , NULL , dspmgr_send_dspkeepalive , NULL);
		//
		pthread_create(&tid , NULL , dspmgr_show_list , NULL);
		pthread_create(&tid1 , NULL , hpvc_client_thread , NULL);

		dspmgr_avsvr();	
		//dspmgr_cli_stop(0xc0a801bd);
		return 0;
}


void test_list()
{
		unsigned int i;

		dspmgr_mem_init();		

		for(i = 10 ; i > 0 ; i--)
		{
				dspmgr_back_dsp(i);	
		}

		for(i = 0 ; i < 10 ; i++)
		{
				unsigned int x;
				if(!dspmgr_get_dsp(&x))
						printf("xxxxxxxxxxxx = %d\n",x);
		}

		for(i = 0 ; i < 10 ; i++)
		{	dspmgr_back_dsp(i);	
		}

}
