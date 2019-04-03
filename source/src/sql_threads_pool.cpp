
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include "../include/sql_threads_pool.h"

#define MAX_THREAD_NUMS      3

#define NOT_READY_WRITE_SQL  0
#define READY_WRITE_SQL      1

#define NOT_SHUTDOWN         0
#define SHUTDOWN             1


uchar g_ready_write_sql = NOT_READY_WRITE_SQL;


Sql_Thread::Sql_Thread()
{
	sql_pool = NULL;
}

Sql_Thread::~Sql_Thread()
{
	if(NULL == sql_pool)
	{
		destory_sql_pool();
	}
}

int Sql_Thread::init_sql_pool(void)
{
	if(NULL == sql_pool)
	{
		sql_pool = (SqlPool_T*)malloc(sizeof(SqlPool_T));
		if(NULL == sql_pool)
		{
			return -1;
		}
	}

	sql_pool->sql_task_num = 0;
	sql_pool->sql_shutdown = NOT_SHUTDOWN;
	sql_pool->sql_max_thread_num = MAX_THREAD_NUMS;	
	sql_pool->sql_head = sql_pool->sql_tail = NULL;

	pthread_mutex_init(&(sql_pool->sql_mutex), NULL);
	pthread_cond_init(&(sql_pool->sql_cond), NULL);

	sql_pool->sql_tids = (pthread_t*)malloc(sizeof(pthread_t));
	if(NULL == sql_pool->sql_tids)
	{
		return -1;
	}

	uchar unCnt = 0;
	for( ; unCnt < MAX_THREAD_NUMS; unCnt++)
	{
		pthread_create(&(sql_pool->sql_tids[unCnt]), NULL, handle_sql_task, this);
	}

	return 0;
	
}

int Sql_Thread::destory_sql_pool(void)
{
	if(NULL == sql_pool)
	{
		return -1;
	}

	if(SHUTDOWN == sql_pool->sql_shutdown)
	{
		return -1;
	}

	sql_pool->sql_shutdown = SHUTDOWN;
	
	pthread_mutex_lock(&(sql_pool->sql_mutex));
	pthread_cond_broadcast(&(sql_pool->sql_cond));
	pthread_mutex_unlock(&(sql_pool->sql_mutex));
	
	uchar unCnt = 0;
	for( ; unCnt < sql_pool->sql_max_thread_num; unCnt++)
	{		
		pthread_join(sql_pool->sql_tids[unCnt], NULL);		
	}

	free(sql_pool->sql_tids);
	sql_pool->sql_tids = NULL;

	SqlTask_T *head = NULL;
	while(NULL != sql_pool->sql_head)
	{
		head = sql_pool->sql_head;
		sql_pool->sql_head = head->next;
		free(head);
	}

	sql_pool->sql_head = sql_pool->sql_tail = NULL;

	pthread_mutex_destroy(&(sql_pool->sql_mutex));
	pthread_cond_destroy(&(sql_pool->sql_cond));

	
	free(sql_pool);
	sql_pool = NULL;



	printf("destory sql thread pool finish\n");
	
	return 0;
	
}

void Sql_Thread::set_mutex_lock(void)
{
	pthread_mutex_lock(&(sql_pool->sql_mutex));
}

void Sql_Thread::set_mutex_unlock(void)
{
	pthread_mutex_unlock(&(sql_pool->sql_mutex));
}

void Sql_Thread::wait_cond_signal(void)
{	
	while(g_ready_write_sql == NOT_READY_WRITE_SQL)
	{
		pthread_cond_wait(&(sql_pool->sql_cond), &(sql_pool->sql_mutex));
	}
	
	while(NOT_SHUTDOWN == sql_pool->sql_shutdown && 0 == sql_pool->sql_task_num)
	{	
		printf("thread: 0x%x waitting\n", (uint)pthread_self());		
		pthread_cond_wait(&(sql_pool->sql_cond), &(sql_pool->sql_mutex));
	}		
}

int Sql_Thread::judge_exit(void)
{	
	if(SHUTDOWN == sql_pool->sql_shutdown && 0 == sql_pool->sql_task_num)
	{
		pthread_mutex_unlock(&(sql_pool->sql_mutex));
		return 0;
	}

	return -1;
}


void *Sql_Thread::handle_sql_task(void *arg)
{
	printf("starting thread : 0x%x\n", (uint)pthread_self());

	Sql_Thread *prove_sql_tids = (Sql_Thread*)arg;
	
	while(1)
	{				
		prove_sql_tids->set_mutex_lock();		
		prove_sql_tids->wait_cond_signal();
		
		if(0 == prove_sql_tids->judge_exit())
		{
			printf("thread 0x%x exit\n", (uint)pthread_self());
			pthread_exit(NULL);
		}
	
		
		SqlTask_T *sql_task = NULL;
		prove_sql_tids->get_sql_task(&sql_task);
		prove_sql_tids->set_mutex_unlock();
			
		if(NULL != sql_task)
		{			
			(sql_task->handle_sql_data)(sql_task->arg);

			if(NULL != sql_task->arg)
			{
				free(sql_task->arg);
				sql_task->arg = NULL;
			}
			
			free(sql_task);
			sql_task = NULL;
		}

		usleep(10);	
		
	}

	pthread_exit(NULL);
	return NULL;
	
}

void *Sql_Thread::handle_sql_data(void *arg)
{
	SqlRecords *info = (SqlRecords*)arg;
	printf("thread 0x %x : cur_col: %u\n\n", (uint)pthread_self(), info->cur_col);
	return NULL;
}


void Sql_Thread::insert_task_to_list(SqlTask_T *task)
{

	if(NULL == task)
	{
		return;
	}

	if(NULL == sql_pool->sql_head)
	{
		sql_pool->sql_head = sql_pool->sql_tail = task;
	}

	else
	{		
		sql_pool->sql_tail->next = task;
		sql_pool->sql_tail = task;
		
	}
	
}


void Sql_Thread::echo(void)
{
	SqlTask_T *prev = sql_pool->sql_head;
	while(prev)
	{
		SqlRecords *val = (SqlRecords*)(prev->arg);
		printf("%u ", val->cur_col);
		prev = prev->next;
	}

	printf("\n\n");
}


int Sql_Thread::add_sql_task(void *arg)
{
	SqlTask_T *sql_task = (SqlTask_T*)malloc(sizeof(SqlTask_T));
	if(NULL == sql_task)
	{
		return -1;
	}

	sql_task->arg = arg;
	sql_task->next = NULL;
	sql_task->handle_sql_data = handle_sql_data;

	insert_task_to_list(sql_task);
	//echo();

	pthread_mutex_lock(&(sql_pool->sql_mutex));
	sql_pool->sql_task_num++;
	insert_task_to_list(sql_task);
	
	pthread_mutex_unlock(&(sql_pool->sql_mutex));

	if(g_ready_write_sql == READY_WRITE_SQL)
	{
		pthread_cond_broadcast(&(sql_pool->sql_cond));
	}

	return 0;


	
}



int Sql_Thread::get_sql_task(SqlTask_T **task)
{
	if(NULL == sql_pool->sql_head)
	{
		*task = NULL;
		return 0;
	}
	
	*task = sql_pool->sql_head;
		
	sql_pool->sql_task_num--;
	
	if(sql_pool->sql_task_num < 0)
	{
		sql_pool->sql_shutdown = SHUTDOWN;
		return -1;
	}
	
	else
	{
		if(sql_pool->sql_task_num == 0)
		{			
			sql_pool->sql_head = sql_pool->sql_tail = NULL;
		}
		
		else
		{
			sql_pool->sql_head = (*task)->next;
		}
		return 0;
	}
}


int main(int argc, char const *argv[])
{
	Sql_Thread *sql_tid = new Sql_Thread();

	sql_tid->init_sql_pool();

	uchar ucCnt = 0;
	for(; ucCnt < 30; ucCnt++)
	{
		SqlRecords *info = (SqlRecords*)malloc(sizeof(SqlRecords));
		if(NULL == info)
		{
			return -1;
		}
		
		info->cur_col = ucCnt+1;		
		sql_tid->add_sql_task((void*)info);
		
		if(5 == ucCnt)
		{
			g_ready_write_sql = READY_WRITE_SQL;
		}
		
		usleep(20);
	}

	printf("\n\nding ni de fei\n\n");
	sql_tid->destory_sql_pool();
	delete sql_tid;

	return 0;
}



