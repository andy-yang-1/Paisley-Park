#include <sys/sdt.h>
#include "liburing.h"
#include <aio.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#define debug

#define FILE_SIZE 16384
#define ITERATION_SUM 512
#define BUFFER_SIZE 4096


int fd = -1 ;

char log_msg[1000000] ;


void error(const char *msg){
    printf("error:%s\n",msg) ;
    exit(1) ;
}

void debug_show(const char *msg){  
#ifdef debug
    printf("%s",msg) ;
#endif
}

void init(){
    FILE* pid_file = fopen("pid_file","w") ;
    fprintf(pid_file,"%d",getpid()) ;
    fclose(pid_file) ;
    printf("pid:%d\n",getpid()) ;
    fd = open("test_file",O_RDWR|O_CREAT) ; // tb
    ftruncate(fd,FILE_SIZE) ;
}

void test_io_uring(){

    
    char write_buf[BUFFER_SIZE] = {"Can you give me one last kiss?\n"} , read_buf[BUFFER_SIZE] ;


    for (int i = 0 ; i < ITERATION_SUM ; i++){

        struct io_uring ring ;
        struct io_uring_sqe *sqe ;
        struct io_uring_cqe *cqe ;
        struct io_uring_cqe* cqe_batch[32] ;


        struct iovec in_task , out_task ;
        out_task.iov_base = write_buf ;
        out_task.iov_len = BUFFER_SIZE ;
        in_task.iov_base = read_buf ;
        in_task.iov_len = BUFFER_SIZE ;

        int ret = io_uring_queue_init(128,&ring,IORING_SETUP_SQPOLL) ;

        if (ret < 0 ){
            error("test_io_uring init ring fail") ;
        }


        // build task queue
        for (int j = 0 ; j < 32 ; j++){

            sqe = io_uring_get_sqe(&ring) ;
            lseek(fd,0,SEEK_SET) ;

            if (j%2==0){ // write
                io_uring_prep_writev(sqe,fd,&out_task,1,0) ;
            }else{ // read
                io_uring_prep_readv(sqe,fd,&in_task,1,0) ;
            }

        }


        // submit the tasks
        int fin_size = 0 , tmp_fin = 0 ;
        while (fin_size < 32){
            fin_size += io_uring_submit(&ring) ;
        }

        

        // harvest the results
        fin_size = 0 ;
        while (fin_size < 32){
            tmp_fin = io_uring_peek_batch_cqe(&ring,cqe_batch,32) ;
            if (tmp_fin==0) continue ;
            // mind the kernel
            io_uring_cq_advance(&ring,tmp_fin) ;
            fin_size += tmp_fin ;



            // validate the results
            for (int j = 0 ; j < tmp_fin ; j++){
                if (cqe_batch[j]->res < 0){
                    error("test_io_uring validation fail") ;
                }
            }
            
        }


        io_uring_queue_exit(&ring) ;
        
    }
    
    
}

void test_blockio(){

    char write_buf[BUFFER_SIZE] = {"Can you give me one last kiss?\n"} , read_buf[BUFFER_SIZE] ;
//    int reg , blockio_call , blockio_ret ;

    for( int i = 0 ; i < ITERATION_SUM ; i++ ){

        for (int j = 0 ; j < 32 ; j++){

            lseek(fd,0,SEEK_SET) ;
            if (j%2==0){ // write
                DTRACE_PROBE(reg,blockio_call) ;
                write(fd,write_buf,BUFFER_SIZE) ;
                DTRACE_PROBE(reg,blockio_ret) ;
            }else{ // read
                DTRACE_PROBE(reg,blockio_call) ;
                read(fd,read_buf,BUFFER_SIZE) ;
                DTRACE_PROBE(reg,blockio_ret) ;
            }

        }

    }


}




int main(){
    init() ;
    sleep(5) ; // let bpf_run read the pid 
    test_io_uring() ;
    test_blockio() ;
    printf("main finished\n") ;
    return 0 ;
}