#include<stdio.h>
#include <unistd.h>
#include <pthread.h>
#include<fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_THREAD 10  //宏定义最大线程数量

						//定义结构体
typedef struct{
    char thread_name[3];		 //线程名称
    unsigned int require_moment; //申请时间
    unsigned int persist_time;   //持续使用时间
    unsigned int i;				 //与require_moment相同
    unsigned int b;				 //开始时间
    unsigned int e;				 //结束时间
}TEST_INFO;

								 //定义测试数据
TEST_INFO test_data[MAX_THREAD]={
    {"r1",13,1},
    {"w1",11,14},
    {"w2",2,5},
    {"w3",9,12},
    {"r2",15,2},
    {"r3",6,9},
    {"w4",12,15},
    {"r4",3,6},
    {"r5",9,12},
    {"w5",0,3}
};

char studentID[8];
char r_seq[MAX_THREAD][3];		//请求队列
char o_seq[MAX_THREAD][3];		//实际运行队列
int read_count=0;				//读者数量
int write_count=0;				//写者数量
int r_seq_count;				//请求队列数量
int o_seq_count;				//实际运行队列数量
pthread_mutex_t CS_DATA;		//静态初始化互斥锁
pthread_mutex_t h_mutex_read_count; //静态初始化互斥锁
pthread_mutex_t h_mutex_write_count; //静态初始化互斥锁
pthread_mutex_t h_mutex_wait;		//静态初始化互斥锁
pthread_mutex_t h_mutex_r_seq_count;//静态初始化互斥锁
pthread_mutex_t h_mutex_o_seq_count;//静态初始化互斥锁

time_t base;//定义UNIX时间戳
/*
    无优先（先到先服务）算法的读者处理函数
*/
void* FIFO_reader_thread(void *data){
    char thread_name[3];
    struct timeval t;
    strcpy(thread_name,((TEST_INFO *)data)->thread_name);

    sleep(((TEST_INFO *)data)->require_moment);
    gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->i = t.tv_sec - base;
    pthread_mutex_lock(&h_mutex_r_seq_count);
        strcpy(r_seq[r_seq_count++], thread_name);
        pthread_mutex_unlock(&h_mutex_r_seq_count);
    pthread_mutex_lock(&h_mutex_wait);//进入申请资源队列
    pthread_mutex_lock(&h_mutex_read_count);
    read_count++;
    if(read_count==1)
        pthread_mutex_lock(&CS_DATA);//是否可以获得文件资源
    pthread_mutex_unlock(&h_mutex_read_count);
    pthread_mutex_unlock(&h_mutex_wait);

    gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->b = t.tv_sec - base;//计算开始时间
        pthread_mutex_lock(&h_mutex_o_seq_count);
        strcpy(o_seq[o_seq_count++], thread_name);
        pthread_mutex_unlock(&h_mutex_o_seq_count);

    sleep(((TEST_INFO *)data)->persist_time);

    gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->e = t.tv_sec - base;//计算结束时间
    pthread_mutex_lock(&h_mutex_read_count);
    read_count--;
    if(read_count==0)
        pthread_mutex_unlock(&CS_DATA);
    pthread_mutex_unlock(&h_mutex_read_count);
    return 0;
}

/*
    无优先（先到先服务）算法的写者处理函数
*/
void* FIFO_writer_thread(void *data){
    struct timeval t;
    sleep(((TEST_INFO *)data)->require_moment);
    gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->i = t.tv_sec - base;
    pthread_mutex_lock(&h_mutex_r_seq_count);
        strcpy(r_seq[r_seq_count++], ((TEST_INFO *)data)->thread_name);
        pthread_mutex_unlock(&h_mutex_r_seq_count);
    pthread_mutex_lock(&h_mutex_wait);
    pthread_mutex_lock(&CS_DATA);
    gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->b = t.tv_sec - base;
    pthread_mutex_lock(&h_mutex_o_seq_count);
        strcpy(o_seq[o_seq_count++], ((TEST_INFO *)data)->thread_name);
        pthread_mutex_unlock(&h_mutex_o_seq_count);
    sleep(((TEST_INFO *)data)->persist_time);
    gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->e = t.tv_sec - base;
    pthread_mutex_unlock(&CS_DATA);
    pthread_mutex_unlock(&h_mutex_wait);
    return 0;
}

/*
    无优先（先来先服务）算法
*/
void first_come_first_served(){
    int i=0;
    r_seq_count = 0;
    o_seq_count = 0;
    pthread_t h_thread[MAX_THREAD];
    struct timeval start_t; //用于表示开始事件
        gettimeofday(&start_t, NULL);//将当前时间记录至start_t中
        base = start_t.tv_sec;//将当前时间的秒数赋值给base
        for(i=0;i<MAX_THREAD;i++){
                printf("%4s\t%3d\t%3d\n",test_data[i].thread_name, test_data[i].require_moment, test_data[i].persist_time);
        };

    pthread_mutex_init(&CS_DATA,NULL);

    for(i=0;i<MAX_THREAD;i++){
        if(test_data[i].thread_name[0]=='r')
            pthread_create(&h_thread[i],NULL,FIFO_reader_thread,&test_data[i]);
        else
            pthread_create(&h_thread[i],NULL,FIFO_writer_thread,&test_data[i]);
    }

    for(i=0;i<MAX_THREAD;i++){
        pthread_join(h_thread[i],NULL);
    }
        printf("\n");
    //refresh();

}
//保存结果致txt文件
void save_answer(FILE *f, char *file_name, char *problem_name) {
    int i;
    fprintf(f, "\t%s\n\tr/w  problem:%s\n\n", file_name, problem_name);
    fprintf(f, "name\tr_m\tp_t\ti_t\tb_t\te_t\t\n");
    for(i = 0; i < MAX_THREAD; i++) {
        fprintf(f, "%4s\t%3d\t%3d\t%3d\t%3d\t%3d\t\n", test_data[i].thread_name, test_data[i].require_moment, test_data[i].persist_time, test_data[i].i, test_data[i].b, test_data[i].e);
    }
    fprintf(f, "\n");
    fprintf(f, "r_seq:");
    for(i = 0; i < MAX_THREAD; i++) {
        fprintf(f, "%4s", r_seq[i]);
    }
    fprintf(f, "\n");
    fprintf(f, "o_seq:");
    for(i = 0; i < MAX_THREAD; i++) {
        fprintf(f, "%4s", o_seq[i]);
    }
    fprintf(f, "\n");
    close(f);
}
int main(int argc,char *argv[]){
    char file_name[100];
    char problem_name[100];
    int *f;
    strcpy(studentID, "20152532");
    first_come_first_served();
        sprintf(file_name, "%s_FCFS_answer.txt", studentID);
        sprintf(problem_name, "FCFS");
        if((f=fopen(file_name, "w")) == NULL) {
            printf("ERROR: fail to open file:%s\n", file_name);
                exit(3);
        }
        save_answer(f, file_name, problem_name);
    return 0;
}
