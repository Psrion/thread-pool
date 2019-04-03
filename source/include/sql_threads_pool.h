
#ifndef __SQL_THREADS_POOL_H__
#define __SQL_THREADS_POOL_H__

#include <pthread.h>

typedef unsigned char uchar;
typedef unsigned int  uint;

typedef struct tag_SqlRecords
{
	uchar cur_col;    /* Sqlite database column number */
	char times[11];   /* Sqlite current time */
	char file[32];    /* Sqlite current file name */
	int  cnts[24];    /* Sqlite current value for every 24 hours */
	time_t cur_sec;   /* Sqlite current total seconds for time */
	struct tag_SqlRecords *next;

}SqlRecords;


typedef struct tag_SqlTask
{
	void *(*handle_sql_data)(void *arg);  /* Handle sqlite task */
	void *arg;                            /* Parameter for sqlite task */
	struct tag_SqlTask *next;             /* The next task pointer */
	
}SqlTask_T;


typedef struct tag_SqlPool
{
	uchar sql_shutdown;        /* Destroy sqlite pool flag */
	uchar sql_max_thread_num;  /* Maximum number of threads which are opened */
	int sql_task_num;         /* Current sqlite task numbers */
	
	pthread_mutex_t sql_mutex; /* Thread mutex */
	pthread_cond_t sql_cond;   /* Thread cond */
	pthread_t *sql_tids;	   /* Thread tid set */
	SqlTask_T *sql_head;       /* Task list head */  
	SqlTask_T *sql_tail;	   /* Task list tail */
	
}SqlPool_T;


class Sql_Thread
{
	public:
		Sql_Thread();
		~Sql_Thread();

	public:
		int init_sql_pool(void);
		int destory_sql_pool(void);
		void echo(void);
		void wait_cond_signal(void);
		int judge_exit(void);

		void set_write_sql_state(int state);
		void set_mutex_lock(void);
		void set_mutex_unlock(void);
		int get_sql_task(SqlTask_T **task);
		int get_write_sql_state(void);
				
		static void *handle_sql_task(void *arg);
		static void *handle_sql_data(void *arg);
		int add_sql_task(void *arg);
		void insert_task_to_list(SqlTask_T *task);

	private:
		SqlPool_T *sql_pool;

};


#endif //__SQL_THREADS_POOL_H__
