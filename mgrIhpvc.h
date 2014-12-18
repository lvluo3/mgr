#define MEMSIZE 6

struct node_t
{
	void * data;
	struct node_t * next;
};

struct list_t
{
	struct node_t * head;
	struct node_t * tail;
	int		size;
};


typedef int (*FILLDATA)(void * pdata);
typedef int (*EMPTYDATA)(void * pdata);

struct hpvc_usr_t
{
	struct list_t	mem_list; 	//empty list
	struct list_t	fifo;	 	//fill	list
	void	*	p;		//data will fill in mem
	FILLDATA	filldata;	//fill data
	EMPTYDATA	emptydata;	//empet data
};
