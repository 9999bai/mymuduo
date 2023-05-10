#include "/usr/include/mymuduo/ThreadPool.h"

#include <mutex>
#include <unistd.h>
#include <condition_variable>

int count_ = 1;
std::mutex mutex_;
std::condition_variable cond_;

void countDown()
{
    std::unique_lock<std::mutex> lock(mutex_);
    count_--;
    if(count_ == 0)
    {
        cond_.notify_all();
    }
}

void wait1()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while(count_ > 0)
    {
        cond_.wait(lock);
    }
}

void print()
{
    usleep(10);
    LOG_INFO("print... currentThread ID = %d", CurrentThread::tid());
}

void printStr(std::string& str)
{
    LOG_INFO("printStr std=%s, currentThread=%d", str.c_str(), CurrentThread::tid());
}

void printid(int i)
{
    LOG_INFO("printid id=%d, currentThread=%d", i, CurrentThread::tid());
}

void test(int maxsize)
{
    LOG_INFO("test.....maxsize=%d\n", maxsize);

    ThreadPool pool("testThreadPoll");
    pool.setMaxQueueSize(maxsize);
    pool.start(5);

    LOG_INFO("add...");
    pool.run(print);
    pool.run(print);
    for (int i = 0; i < 100; i++)
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "task %d",i);
        pool.run(std::bind(printStr, std::string(buf)));
    }
    pool.run(std::bind(countDown));
    wait1();

    LOG_INFO("down...");
    pool.stop();
}

void test2()
{
    LOG_INFO("test2...");
    ThreadPool pool("test2 Threadpool...");
    pool.setMaxQueueSize(5);
    pool.start(3);

    Thread th1([&pool]()
    {
        for(int i=0; i<10; i++)
        {
            pool.run(std::bind(printid, i));
        } 
    },"thread");
    th1.start();
    sleep(5);
    th1.join();
    pool.run(print);
    LOG_INFO("test2 down...");
}

int main()
{
    test(1);
    LOG_INFO("....1.....");
    test(0);
    LOG_INFO("....2.....");
    test(5);
    LOG_INFO("....3.....");
    test(10);
    LOG_INFO("....4.....");
    test(50);

    test2();
}
