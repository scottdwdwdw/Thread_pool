
#include<iostream>
#include<unistd.h>
#include<algorithm>
#include"thread_pool.h"
#include"debug.h"




namespace THREAD_POOL
{


void* test_task(void*a)
 {
 	std::cout<<__FUNCTION__<<" "<<pthread_self()<<std::endl;


 	return NULL;
 }

void* test_task2(void*a)
{
	std::cout<<__FUNCTION__<<std::endl;
	return NULL;
}

void master_thread(thread_pool*pool)      //work interface
 {
 	__DEBUG(("[%s]I am in master,%lu",__FUNCTION__,pthread_self()));
 	
 	while(!pool->get_thread_state())
 	{
 		pool->push_task(test_task);
 		__DEBUG(("[%s]idle num is : %d",__FUNCTION__,pool->get_idle_num()));
 		pool->assign_task(test_task,NULL);

 		pool->start_task();
 		
 		sleep(2);
 		
 	}


 	__DEBUG(("[%s]thread termainal",__FUNCTION__));

 }



  thread::thread():thread_id(0),process_fun(NULL),arg(NULL),thread_state(IDLE)
  {

  }


  thread_pool::thread_pool():norm_thead_num(NORM_THREAD_NUM),
						    max_thread_num(MAX_THREAD_NUM),
						    task_queue(),
						    act_thread(),
						    idle_thread(),
						    stop_flag(FALSE)
  {
		(void)pthread_attr_init(&attr);
		(void)pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
		(void)pthread_mutex_init(&task_mutex,NULL);
		(void)pthread_mutex_init(&act_thread_mutex,NULL);
		(void)pthread_mutex_init(&idle_thread_mutex,NULL);
		(void)pthread_cond_init(&task_cond,NULL);
		(void)pthread_cond_init(&idle_cond,NULL);
		(void)pthread_cond_init(&term_cond,NULL);

		(void)create_threads(norm_thead_num);           //create thread pool

 }

 thread_pool::thread_pool(int n):norm_thead_num(n),
						    max_thread_num(MAX_THREAD_NUM),
						    task_queue(),
						    act_thread(),
						    idle_thread(),
						    stop_flag(FALSE)
 {
 		(void)pthread_attr_init(&attr);
		(void)pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
		(void)pthread_mutex_init(&task_mutex,NULL);
		(void)pthread_mutex_init(&act_thread_mutex,NULL);
		(void)pthread_mutex_init(&idle_thread_mutex,NULL);
		(void)pthread_cond_init(&task_cond,NULL);
		(void)pthread_cond_init(&idle_cond,NULL);
		(void)pthread_cond_init(&term_cond,NULL);

		(void)create_threads(norm_thead_num);
 }


 int thread_pool::create_threads(int num) 
 {
 	pthread_mutex_lock(&idle_thread_mutex);
 	for(int i=0;i<num;++i)
 	{
 		
 		thread* tp=new thread;
 		pthread_create(&tp->thread_id,&attr,(task)master_thread,this);  //why
 		__DEBUG(("[%s]create thread %lu",__FUNCTION__,tp->get_threadID()));
 		tp->thread_state=IDLE;
 		idle_thread.push_back(tp);       //add idle thread
 		//idle_thread.insert(tp);   
 	}
 	pthread_mutex_unlock(&idle_thread_mutex);

 	pthread_cond_signal(&idle_cond);     //we have idle thread
 	return idle_thread.size();
 }

 void thread_pool::push_task(task t)
 {
 	pthread_mutex_lock(&task_mutex);
 	task_queue.push_back(t);
 	pthread_mutex_unlock(&task_mutex);

 	//pthread_cond_signal(&task_cond);        //signal to the thread --- task is ready
 }


 void thread_pool::assign_task(task t,void*arg)
 {
 	pthread_mutex_lock(&idle_thread_mutex);
 	pthread_mutex_lock(&act_thread_mutex);

    while(idle_thread.size()==0&&!stop_flag)   //there is no other  idle thread,wait
    {
    	__DEBUG(("[%s] no idle thread.",__FUNCTION__));
    	pthread_cond_wait(&idle_cond,&idle_thread_mutex);   //wait
    }

 	auto work_thread = idle_thread.back();
 	work_thread->process_fun=t;
 	work_thread->arg = arg;
 	work_thread->thread_state=BUSY;

 	act_thread.push_back(work_thread);
 	idle_thread.pop_back();
 	task_queue.pop_back();
 	__DEBUG(("[%s]task queue is: %d",__FUNCTION__,get_task_num()));

 	pthread_mutex_unlock(&idle_thread_mutex);
 	pthread_mutex_unlock(&act_thread_mutex);


 }

inline void thread_pool::start_task()
{
	pthread_mutex_lock(&act_thread_mutex);
	auto work_thread = act_thread.back();
	pthread_mutex_unlock(&act_thread_mutex);

	work_thread->process_fun(work_thread->arg);

	finish_task(work_thread);
}

inline void thread_pool::finish_task(thread* work_thread)
{
	pthread_mutex_lock(&idle_thread_mutex);
 	pthread_mutex_lock(&act_thread_mutex);

 

 	work_thread->process_fun=NULL;
 	work_thread->arg=NULL;
 	work_thread->thread_state=IDLE;

 	idle_thread.push_back(work_thread);
 	auto it=std::find(act_thread.begin(),act_thread.end(),work_thread);
 	   __DEBUG(("[%s],idle %lu,finish",__FUNCTION__,(*it)->get_threadID()));
 	act_thread.erase(it);
 	__DEBUG(("[%s]act idle is :%d",__FUNCTION__,act_thread.size()));

 	pthread_mutex_unlock(&idle_thread_mutex);
 	pthread_mutex_unlock(&act_thread_mutex);

 	pthread_cond_signal(&idle_cond);
 	if(act_thread.size()==0)pthread_cond_signal(&term_cond);
}



void thread_pool::terminate_all()
{
	pthread_mutex_lock(&idle_thread_mutex);
 	pthread_mutex_lock(&act_thread_mutex);

 	if(act_thread.size()!=0)
 	{
 		for(auto c:act_thread)
 		{
 			c->process_fun=NULL;
 			c->arg=NULL;
 			c->thread_state=IDLE;
 			idle_thread.push_back(c);
 		}
 		act_thread.clear();
 	}

 	idle_thread.clear();
 	task_queue.clear();

 	stop_flag=TRUE;
 	pthread_mutex_unlock(&idle_thread_mutex);
 	pthread_mutex_unlock(&act_thread_mutex);

 	pthread_mutex_destroy(&task_mutex);
 	pthread_mutex_destroy(&act_thread_mutex);
 	pthread_mutex_destroy(&idle_thread_mutex);
 	pthread_cond_destroy(&task_cond);
 	pthread_cond_destroy(&idle_cond);
 	pthread_cond_destroy(&term_cond);
 	pthread_attr_destroy(&attr);
}








}