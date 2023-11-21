#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
void Worker(int cnt)
{
    printf("I am child,pid=%d,cnt=%d\n", getpid(), cnt);
}

int main()
{
    pid_t id = fork();
    if (id == 0)
    {
        // child
        int cnt = 5;
        while (cnt)
        {
            Worker(cnt);
            sleep(2);
            cnt--;
        }
        exit(0);
    }

    // 要进行非阻塞轮询检测
    // 一般要结合while
    while (1)
    {
        // father do
        int status = 0;
        // WNOHANG -> 非阻塞式等待
        pid_t rid = waitpid(id, &status, WNOHANG);
        if (rid > 0)
        {
            // 等待成功
            // status 为子进程的退出状态
            // 高8位为退出信息，低七位为退出信号
            printf("child quit success,exit code: %d,exit sina;:%d\n", (status >> 8) & 0xFF, status & 0x7F);
            break;
        }
        else if (rid == 0)
        {
            // 非阻塞等待，rid为0时，表示等待成功，但是子进程还没有退出
            printf("chile is alive,father do other thing......\n");
        }
        else
        {
            // 等待失败，子进程状况无法获取
            printf("wait failed!\n");
            break;
        }
        sleep(1);
    }
}