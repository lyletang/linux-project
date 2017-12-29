#include<stdio.h>
#include <unistd.h>
#include <pthread.h>
#include<fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#define MAX_THREAD 10          //�궨������߳�����

					   
typedef struct{		           //����ṹ��
    char thread_name[3];       //�߳�����
    unsigned int require_moment; //����ʱ��
    unsigned int persist_time; //����ʹ��ʱ��
    unsigned int i;            //������ʱ����ͬ
    unsigned int b;            //��ʼʱ��
    unsigned int e;            //����ʱ��
}TEST_INFO;


TEST_INFO test_data[MAX_THREAD]={	//�����������
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
char r_seq[MAX_THREAD][3];//�������
char o_seq[MAX_THREAD][3];//���ж���
int read_count=0;		  //��������
int write_count=0;        //д������
int r_seq_count;          //�����������
int o_seq_count;          //ʵ�����ж�������
pthread_mutex_t CS_DATA;  //��̬��ʼ����������������Դ����
pthread_mutex_t h_mutex_read_count;		//��ʼ��������
pthread_mutex_t h_mutex_write_count;	//��ʼ��������
pthread_mutex_t h_mutex_reader_wait;	//��ʼ��������
pthread_mutex_t h_mutex_first_reader_wait; //��ʼ��������
//pthread_mutex_t h_mutex_wait;			   //��ʼ��������
pthread_mutex_t h_mutex_r_seq_count;	   //��ʼ��������
pthread_mutex_t h_mutex_o_seq_count;	   //��ʼ��������

time_t base;							   //����UNIXʱ���
/*
    д�������㷨�Ķ��ߴ�����
*/
void* WF_reader_thread(void *data){
    char thread_name[3];
    struct timeval t;

    strcpy(thread_name,((TEST_INFO *)data)->thread_name);

    sleep(((TEST_INFO *)data)->require_moment);
    gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->i = t.tv_sec - base;
    
    pthread_mutex_lock(&h_mutex_r_seq_count);
        strcpy(r_seq[r_seq_count++], thread_name);
        pthread_mutex_unlock(&h_mutex_r_seq_count);

    pthread_mutex_lock(&h_mutex_reader_wait);
    pthread_mutex_lock(&h_mutex_first_reader_wait);
    pthread_mutex_lock(&h_mutex_read_count);
    read_count++;
    if(read_count==1)
        pthread_mutex_lock(&CS_DATA);//������Դ
    pthread_mutex_unlock(&h_mutex_read_count);
    pthread_mutex_unlock(&h_mutex_first_reader_wait);
    pthread_mutex_unlock(&h_mutex_reader_wait);

    gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->b = t.tv_sec - base;//���㿪ʼʱ��
        pthread_mutex_lock(&h_mutex_o_seq_count);
        strcpy(o_seq[o_seq_count++], thread_name);
        pthread_mutex_unlock(&h_mutex_o_seq_count);

    sleep(((TEST_INFO *)data)->persist_time);

    gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->e = t.tv_sec - base;//�������ʱ��

    pthread_mutex_lock(&h_mutex_read_count);
    read_count--;
    if(read_count==0)
        pthread_mutex_unlock(&CS_DATA);
    pthread_mutex_unlock(&h_mutex_read_count);
    return 0;
}

/*
    д�������㷨��д�ߴ�����
*/
void* WF_writer_thread(void *data){
    struct timeval t;
    sleep(((TEST_INFO *)data)->require_moment);
    gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->i = t.tv_sec - base;
    pthread_mutex_lock(&h_mutex_r_seq_count);
        strcpy(r_seq[r_seq_count++], ((TEST_INFO *)data)->thread_name);
        pthread_mutex_unlock(&h_mutex_r_seq_count);
    
    pthread_mutex_lock(&h_mutex_write_count);
    if(write_count==0)
        pthread_mutex_lock(&h_mutex_first_reader_wait);
    write_count++;
    pthread_mutex_unlock(&h_mutex_write_count);

    pthread_mutex_lock(&CS_DATA);//������Դ
    

    gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->b = t.tv_sec - base;
        pthread_mutex_lock(&h_mutex_o_seq_count);
        strcpy(o_seq[o_seq_count++], ((TEST_INFO *)data)->thread_name);
        pthread_mutex_unlock(&h_mutex_o_seq_count);
    sleep(((TEST_INFO *)data)->persist_time);
    gettimeofday(&t, NULL);
        ((TEST_INFO *)data)->e = t.tv_sec - base;

    pthread_mutex_unlock(&CS_DATA);

    pthread_mutex_lock(&h_mutex_write_count);
    write_count--;
    if(write_count==0)
        pthread_mutex_unlock(&h_mutex_first_reader_wait);
    pthread_mutex_unlock(&h_mutex_write_count);
    return 0;
}


/*
    д�������㷨
*/
void writer_first(){
    int i=0;
    r_seq_count = 0;
        o_seq_count = 0;
    pthread_t h_thread[MAX_THREAD];
    struct timeval start_t; 
        gettimeofday(&start_t, NULL);
        base = start_t.tv_sec;
        for(i=0;i<MAX_THREAD;i++){
                printf("%4s\t%3d\t%3d\n",test_data[i].thread_name, test_data[i].require_moment, test_data[i].persist_time);
        };


    pthread_mutex_init(&CS_DATA,NULL);

    for(i=0;i<MAX_THREAD;i++){
        if(test_data[i].thread_name[0]=='r')
            pthread_create(&h_thread[i],NULL,WF_reader_thread,&test_data[i]);
        else
            pthread_create(&h_thread[i],NULL,WF_writer_thread,&test_data[i]);
    }
    for(i=0;i<MAX_THREAD;i++){
        pthread_join(h_thread[i],NULL);
    }
        printf("\n");
}

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
    printf("The result is saved in %s\n", file_name);
}
int main(int argc,char *argv[]){
    char file_name[100];
    char problem_name[100];
    int *f;
    strcpy(studentID, "20152535");
    writer_first();
        sprintf(file_name, "%s_writer_first_answer.txt", studentID);
        sprintf(problem_name, "writer_first");
        if((f=fopen(file_name, "w")) == NULL) {
            printf("ERROR: fail to open file:%s\n", file_name);
                exit(1);
        }
        save_answer(f, file_name, problem_name);
    
    return 0;
}
