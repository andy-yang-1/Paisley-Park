## I/O-lib 学习



#### 大纲

- 了解各式各样的 I/O 方法的库与接口
- IO 库
  - io_uring ：使用 liburing 接口
  - blockio
  - glibcio
  - nativeio



#### io_uring

- 初始化

  ~~~c
  struct io_uring ring ;
  struct io_uring_sqe *sqe ;
  struct io_uring_cqe *cqe ;
  struct iovec io_task[n] ;
  
  // 创建一个 io_uring instance , 指定其大小为 BatchSize
  int ret = io_uring_queue_init(BatchSize,&ring,IORING_SETUP_SQPOLL) ;
  ~~~

- 命令传递

  ~~~c
  // 创建任务
  sqe = io_uring_get_sqe(&ring) ;
  // 读任务
  io_uring_prep_readv(sqe,fd,&io_task,1,offset) ;
  // 写任务
  io_uring_prep_writev(sqe,fd,&io_task,1,offset) ;
  ~~~

- 提交任务

  ~~~c
  // 返回完成了的任务数量
  int fin_size = io_uring_submit(&ring) ;
  ~~~

- 收割结果

  ~~~c
  // 让 cqe 收割结果
  ret = io_uring_wait_cqe(&ring,&cqe) ;
  // 批量收割结果
  io_uring_cqe** batch_cqes ;
  fin_size = io_uring_peek_batch_cqe(&ring,batch_cqes,batch_size) ;
  ~~~

- 提醒内核，结果已经被收割

  ~~~c
  io_uring_cqe_seen(&ring,cqe) ;
  io_uring_cq_advance(&ring,batchsize) ;
  ~~~

- 销毁 io_uring

  ~~~c
  io_uring_queue_exit(&ring) ;
  ~~~



