#include "/usr/include/mymuduo/EventLoop.h"
#include "/usr/include/mymuduo/EventLoopThread.h"

#include <stdio.h>
#include <unistd.h>

int cnt = 0;
EventLoop *g_loop;

void printTid()
{
  LOG_INFO("pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
  LOG_INFO("now : %s", Timestamp::now().toString().c_str());
}

void print(const char* msg)
{
  LOG_INFO("msg %s %s\n", Timestamp::now().toString().c_str(), msg);
  if (++cnt == 20)
  {
    g_loop->quit();
  }
}

void cancel(TimerId timer)
{
  g_loop->cancel(timer);
  LOG_INFO("cancelled at %s\n", Timestamp::now().toString().c_str());
}

int main()
{
    printTid();
    {
      // EventLoop loop;
      // g_loop = &loop;

      // print("main");
      // loop.runAfter(1, std::bind(print, "once1"));
      // // loop.runAfter(1.5, std::bind(print, "once1.5"));
      // // loop.runAfter(2.5, std::bind(print, "once2.5"));
      // // loop.runAfter(3.5, std::bind(print, "once3.5"));
      // // TimerId t45 = loop.runAfter(4.5, std::bind(print, "once4.5"));
      // // loop.runAfter(4.2, std::bind(cancel, t45));
      // // loop.runAfter(4.8, std::bind(cancel, t45));
      // loop.runEvery(2, std::bind(print, "every2"));
      // TimerId t3 = loop.runEvery(3, std::bind(print, "every3"));
      // loop.runAfter(9.001, std::bind(cancel, t3));

      // loop.loop();
      // LOG_INFO("main loop exits");

    sleep(1);
    {
      EventLoopThread loopThread;
      EventLoop* loop = loopThread.startLoop();
      loop->runAfter(2, printTid);
      sleep(3);
      LOG_INFO("thread loop exits");
    }

  }
}

