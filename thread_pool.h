#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include<vector>
#include<deque>
#include<set>
#include<pthread.h>


namespace THREAD_POOL
{

	class thread_pool ;				//forward declaration
	extern void master_thread(thread_pool*pool);
	#define NORM_THREAD_NUM 5
	#define MAX_THREAD_NUM  0
	enum{FALSE,TRUE};
	enum{BUSY,IDLE};      //thread state
	typedef void*(*task)(void*);

   

	class thread          			//single thread information
	{
	public:
		friend class thread_pool;
	public:
		thread();
		void run();        
		pthread_t &get_threadID(){return thread_id;}
	private:
		pthread_t thread_id;     	//process id
		task process_fun;
		void* arg;   				//process function arg
		int thread_state;    
	};

	class thread_pool                       
	{
	public:
		thread_pool();               		//default construct
		thread_pool(int n);    
		~thread_pool()
		{

			terminate_all();
		}            		
		//member function
		int create_threads(int num); 
		void push_task(task t);
		void assign_task(task t,void*arg);           //a task connect to a thread
		void start_task();
		void finish_task(thread* work_thread);		 //complete task ,return the thead to the pool
		size_t get_task_num() const {return task_queue.size();} 
		size_t get_idle_num() const {return idle_thread.size();}
		size_t get_act_num() const {return act_thread.size();}
		void set_thread_state(bool state){stop_flag=state;}
		bool get_thread_state()const {return stop_flag;}

		void terminate_all();                        //close all threads


	private:
		int norm_thead_num;
		int max_thread_num;   
		std::deque<task>task_queue;                  //task queue
		std::vector<thread*>act_thread;              //activity threads
		std::vector<thread*>idle_thread;             //idle threads
		    
		pthread_attr_t attr;
		pthread_mutex_t task_mutex;
		pthread_mutex_t act_thread_mutex;
		pthread_mutex_t idle_thread_mutex;
		pthread_cond_t  task_cond;
		pthread_cond_t  idle_cond;
		pthread_cond_t  term_cond;
		bool stop_flag;

	};



}



#endif  //!_THREAD_POOL_H