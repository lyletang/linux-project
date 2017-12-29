#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>

#define MAX_THREAD 10	//宏定义最大线程数量
#define SNL 8			//宏定义studentNumber的长度

typedef struct{		    //定义结构体，命名为TEST_INFO
    char tn[3];         //线程名称
    unsigned int rm;    //线程的申请时间
    unsigned int pt;    //线程的持续时间
    unsigned int i;     //与申请时间相同
    unsigned int b;     //开始时间
    unsigned int e;     //结束时间
}TEST_INFO;

typedef struct {				  //定义测试数据
    char sn[SNL+1];               //学号
    TEST_INFO ti[MAX_THREAD];     //测试数据
}TI;                              

TI test_item={{"20152535”},		  //测试数据
				{{"r1”,13,1},
				{“w1”,11,14},
				{"w2”,2,5},
				{"w3”,9,12},
				{"r2",15,2},^M
				{"r3”,2,5},
				{“w4”,12,15},
				{"r4”,3,6},
				{"r5”,9,12},
				{"w5”,0,3}}
};
char r_seq[MAX_THREAD][3];		 //请求队列
char o_seq[MAX_THREAD][3];	     //运行队列
int sr=0;					     //请求队列数量
int so=0;					     //运行队列数量

int rc=0;                        //读者数量
pthread_mutex_t cs_d;            //初始化互斥锁
pthread_mutex_t cs_rc;           //初始化互斥锁
pthread_mutex_t cs_sr;           //初始化互斥锁
pthread_mutex_t cs_so;           //初始化互斥锁
time_t base;                     //定义时间戳


void save_answer(FILE *f){		 //保存结果致txt文件
    int i;

    fprintf(f,"\t%s_answer.txt\n\tr/w  problem:read first\n\n",test_item.sn);
    fprintf(f,"name r_m p_t     i_t     b_t     e_t\n");
    for(i=0;i<MAX_THREAD;i++){
        fprintf(f,"%4s%4d%4d%8d%8d%8d\n",(test_item.ti)[i].tn,(test_item.ti)[i].rm,(test_item).ti[i].pt,(test_item.ti)[i].i,(test_item.ti)[i].b,(test_item.ti)[i].e);
    }

    fprintf(f,"\n");
    fprintf(f,"r_seq:");
    for(i=0;i<MAX_THREAD;i++){
        fprintf(f,"%4s",r_seq[i]);
    }
    fprintf(f,"\n");
    fprintf(f,"o_seq:");
    for(i=0;i<MAX_THREAD;i++){
        fprintf(f,"%4s",o_seq[i]);
    }
    fprintf(f,"\n");
}
/*
	读者优先算法处理读者的函数
*/
void *r(void *td){
    struct timeval t;
    time_t rl=base;
   
  sleep(((TEST_INFO *)td)->rm);
  gettimeofday(&t,NULL);
  ((TEST_INFO *)td)->i=difftime(t.tv_sec,rl);
  pthread_mutex_lock(&cs_sr);
  strcpy(r_seq[sr++],((TEST_INFO *)td)->tn);
  pthread_mutex_unlock(&cs_sr);
 
  pthread_mutex_lock(&cs_rc);
  rc++;
  if(rc==1)pthread_mutex_lock(&cs_d);
  pthread_mutex_unlock(&cs_rc);
 
  gettimeofday(&t,NULL);
  ((TEST_INFO *)td)->b=difftime(t.tv_sec,rl);
  pthread_mutex_lock(&cs_so);
  strcpy(o_seq[so++],((TEST_INFO *)td)->tn);
  pthread_mutex_unlock(&cs_so);
 
  sleep(((TEST_INFO *)td)->pt);
  gettimeofday(&t,NULL);
  ((TEST_INFO *)td)->e=difftime(t.tv_sec,rl);
 
  pthread_mutex_lock(&cs_rc);
  rc--;
  if(rc==0)pthread_mutex_unlock(&cs_d);
  pthread_mutex_unlock(&cs_rc);
  return 0;
}
/*
	读者优先算法处理读者的算法
*/
void *w(void *td){
  struct timeval t;
  time_t wl=base;
  sleep(((TEST_INFO *)td)->rm);
  gettimeofday(&t,NULL);
  ((TEST_INFO *)td)->i=difftime(t.tv_sec,wl);
  pthread_mutex_lock(&cs_sr);
  strcpy(r_seq[sr++],((TEST_INFO *)td)->tn);
  pthread_mutex_unlock(&cs_sr);
 
  pthread_mutex_lock(&cs_d);
  gettimeofday(&t,NULL);
  ((TEST_INFO *)td)->b=difftime(t.tv_sec,wl);
  pthread_mutex_lock(&cs_so);
  strcpy(o_seq[so++],((TEST_INFO *)td)->tn);
  pthread_mutex_unlock(&cs_so);
 
  sleep(((TEST_INFO *)td)->pt);
  gettimeofday(&t,NULL);
  ((TEST_INFO *)td)->e=difftime(t.tv_sec,wl);
 
  pthread_mutex_unlock(&cs_d);
  return 0;
}

void create_exam(){				//创建要测试的进程
    int i=0;
    pthread_t ht[MAX_THREAD];
    pthread_mutex_init(&cs_d,NULL);
    pthread_mutex_init(&cs_rc,NULL);
    pthread_mutex_init(&cs_sr,NULL);
    pthread_mutex_init(&cs_so,NULL);
    struct timeval t;
   
    gettimeofday(&t,NULL);
    base=t.tv_sec;
   
    for(i=0;i<MAX_THREAD;i++){
        if((test_item.ti)[i].tn[0]=='r'){
            pthread_create(&ht[i],NULL,r,&((test_item.ti)[i]));
        }
        else if((test_item.ti)[i].tn[0]=='w'){
            pthread_create(&ht[i],NULL,w,&((test_item.ti)[i]));   
        }
    }
    for(i=0;i<MAX_THREAD;i++){
        pthread_join(ht[i],NULL);
    }
    pthread_mutex_destroy(&cs_d);
    pthread_mutex_destroy(&cs_rc);
    pthread_mutex_destroy(&cs_sr);
    pthread_mutex_destroy(&cs_so);
}

int main(int argc,char *argv[]){
    int i=0;
    int si,pos;
    int fd;
    FILE *fa;
    char file_name[100];

    create_exam();
   
    sprintf(file_name,"%s_answer.txt",test_item.sn);
    if((fa=fopen(file_name,"w"))==NULL){
        printf("Error openning answer file:%s\n",file_name);
        exit(3);
    }
    save_answer(fa);       
    exit(0);
}
