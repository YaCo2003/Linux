#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM 1024
#define SIZE 64
#define SEP " "
// #define Debug 1
int lastcode; // 最后一个进程的退出码

const char *getUsername()
{
    char *name = getenv("USER");
    if (name)
        return name;
    else
        return "none";
}
const char *getHostname()
{
    char *Hostname = getenv("HOSTNAME");
    if (Hostname)
        return Hostname;
    else
        return "none";
}
const char *getPwd()
{
    char *Pwd = getenv("PWD");
    if (Pwd)
        return Pwd;
    else
        return "none";
}
int getUserCommand(char *command, int num)
{
    // 1.打印工作目录
    printf("[%s@%s %s]# ", getUsername(), getHostname(), getPwd());
    // 2.按格式打印
    // 在这里scanf遇到空格就会结束
    // scanf("%s", command);
    char *ret = fgets(command, num, stdin);
    if (ret == NULL)
        return -1;
    // 在你输入完成时，还是会敲回车，会多输入一个"\n",要去除
    // 没可能越界，直接敲回车都会输入"\n"
    command[strlen(command) - 1] = '\0';
    return strlen(command);
}

void commandSplit(char *in, char *out[])
{
    int argc = 0;
    // eg："ls -a -l" -> "ls","-a","-l"
    // 在这里我们按照空格来截取,SEP=' '
    // strtok函数第一次传参数，后面传NULL，截取完自动返回NULL
    out[argc++] = strtok(in, SEP);
    // 写法1：
    // while (out[argc++] = strtok(NULL, SEP));
    // 写法2：
    while (1)
    {
        out[argc] = strtok(NULL, SEP);
        if (out[argc] == NULL)
            break;
        argc++;
    }
#ifdef Debug
    for (int i = 0; out[i]; i++)
    {
        printf("%d:%s\n", i, out[i]);
    }
#endif
}
int execute(char *argv[])
{
    // 创建子进程来执行
    pid_t id = fork();
    if (id < 0) // 创建失败
    {
        return -1;
    }
    else if (id == 0)
    {
        // 子进程
        // 在这里想让子进程执行用户输入的argv
        // 选择程序替换的接口 这里选择execvp
        // 参数1：程序名 参数2：执行的命令
        execvp(argv[0], argv);
        exit(1);
    }
    else
    {
        // 父进程,等待子进程
        // 阻塞式等待
        int status = 0;
        pid_t rid = waitpid(id, &status, 0);
        if (rid > 0) // 正常退出
        {
            lastcode = WEXITSTATUS(status);
        }
        // 有一批命令不能让子进程执行，得让父进程执行 - 内建命令
        // eg: cd .. / export
        // 所以执行命令前要先判断命令的类型
    }
    return 0;
}

char pwd[1024];
void cd(const char *path)
{
    chdir(path);

    // 更改环境变量才能使执行cd时getPwd()改变
    // 这里不能用临时变量，否则函数结束，添加的环境变量就消失了
    // char pwd[1024];
    // 获取当前目录的绝对路径传入,而不是使用 用户传入的path
    // 这里可以时getPwd返回值与调用系统pwd保持一致
    char tmp[1024];
    getcwd(tmp, sizeof(tmp));
    sprintf(pwd, "PWD=%s", tmp); // bug
    putenv(pwd);
}
char enval[1024];
int doBuildIn(char *argv[])
{
    if (strcmp(argv[0], "cd") == 0)
    {
        // 执行cd切换目录--内建命令
        char *path = NULL;
        if (argv[1] == NULL)
        {
            path = ".";
        }
        else
        {
            path = argv[1];
        }
        cd(path);
        return 1;
    }
    else if (strcmp(argv[0], "export") == 0)
    {
        // 执行export导入系统环境变量 -内建命令
        if (argv[1] == NULL)
            return 1;
        // 在这里不能直接导入，否则while执行下一次命令后，导入的环境变量会消失。原因在于while每次执行，usercommand会重新写入
        // 所以操作系统在真正导入环境变量的时候会创建一张非常大的全局二维数组环境变量表来存储，防止丢失
        // 本次实验就创建一张一维数组小表来测试,char enval[1024]
        strcpy(enval, argv[1]);
        putenv(enval);
        return 1;
    }
    else if (strcmp(argv[0], "echo") == 0)
    {
        char *val = argv[1] + 1; //$PATH  $?
        // 查看？查看退出码
        if (strcmp(val, "?") == 0)
        {
            printf("%d\n", lastcode);
            lastcode = 0;
        }
        else // 查看PATH 环境变量
        {
            printf("%s\n", getenv(val));
        }
        return 1;
    }
}

int main()
{
    // 命令行解释器是死循环，一直跑
    while (1)
    {
        char usercommand[NUM]; // 存储输入命令串
        char *argv[SIZE];      // 存储截取后的各个单独命令串

        // 1.获取打印工作目录、获取用户输入命令字符串
        int n = getUserCommand(usercommand, sizeof(usercommand));
        // 这里表示输入要么出错，要么为空，直接跳过继续下一次
        if (n <= 0)
            continue;

        // 2.分割字符串
        // eg："ls -a -l" -> "ls","-a","-l"
        commandSplit(usercommand, argv);

        // 3.判断命令的类型(是否为内建命令)
        n = doBuildIn(argv);
        if (n)
            continue;

        // 4.执行argv[]对应的命令
        execute(argv);
        // printf("echo:%s", usercommand);
    }
    return 0;
}
