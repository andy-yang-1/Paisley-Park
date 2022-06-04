## eBPF 学习



#### 简介

- 本质：向内核注入模块，使得可以在不修改内核的情况下，在内核态运行代码，故可以让内核添加功能
- 功能
  1. 性能跟踪 （we focus on this）
  2. 网络
  3. 容器
  4. 安全
- 架构
  - 用户用 c 编写程序，用 llvm 编译成 eBPF byte code，在 root 权限下调用 bpf() 加载到内核
  - eBPF 对代码进行安全验证，编译成机器码，然后挂载到不同路径上，内核运行到这些路径时执行机器码



#### 安装与使用

- 安装（source code 编译安装）

  ~~~shell
  # https://github.com/iovisor/bcc/blob/master/INSTALL.md#ubuntu---source
  # for 20.04 ubuntu
  
  # config
  sudo apt install -y bison build-essential cmake flex git libedit-dev \
    libllvm12 llvm-12-dev libclang-12-dev python zlib1g-dev libelf-dev libfl-dev python3-distutils
  
  # install and compile
  git clone https://github.com/iovisor/bcc.git
  mkdir bcc/build; cd bcc/build
  cmake ..
  make
  sudo make install
  cmake -DPYTHON_CMD=python3 .. # build python3 binding
  pushd src/python/
  make
  sudo make install
  popd
  ~~~

- 使用（以 hello.c 为例）

  - 需要安插的代码

    ~~~c
    // hello.c 
    int hello_world(struct pt_regs *ctx) // ctx 为 args
    {
        bpf_trace_printk("Hello, World!"); 
        // 将字符串打印到 /sys/kernel/debug/tracing/trace_pipe 文件下
        return 0;
    }
    ~~~

  - 执行插入的代码

    ~~~python
    # hello.py
    # 1) 加载 BCC 库
    from bcc import BPF
    
    # 2) 加载 eBPF 内核态程序
    b = BPF(src_file="hello.c")
    
    # 3) 将 eBPF 程序挂载到 kprobe
    b.attach_kprobe(event="do_sys_openat2", fn_name="hello_world")
    # 当系统执行 do_sys_openat2() 时，调用 hello_world()
    
    # 4) 读取并且打印 eBPF 内核态程序输出的数据
    b.trace_print()
    # 将 /sys/kernel/debug/tracing/trace_pipe 内容打印到标准输出
    ~~~

    

#### 接口

- 加载 BPF

  ~~~python
  # 直接加载字符串
  b = BPF(text="[c_program]")
  # 加载目录下文件
  b = BPF(src_file="[file_path]")
  ~~~

- 加载路径

  ~~~python
  # 在 call syscall (kernel) 时 trap
  b.attach_kprobe(event="[syscall_funcname]",fn_name="[loaded_func]") # 若 loaded_funcname 天然为 kprobe__[syscall_funcname] BPF 后无需加载路径
  
  # 在 syscall (kernel) return 时 trap
  b.attach_kretprobe(event="[syscall_funcname]",fn_name="[loaded_func]")
  
  # 在 call syscall (lib/usr) 时 trap  比如 strlen 是 c 的库 name="c"
  b.attach_uprobe(name="[lib_name]",sym="[syscall_funcname]",fn_name="[loaded_func]")
  
  # 在 syscall (lib/usr) return 时 trap
  b.attach_uretprobe(name="[lib_name]",sym="[syscall_funcname]",fn_name="[loaded_func]")
  
  # 获得 syscall (kernel) 函数名
  syscall_fn_name = b.get_syscall_fnname("[fn_name]")
  ~~~

- 用户层静态探针（user statically-defined tracing -> USDT）

  - 原因

    设计好了的探针都是在 syscall 时调用指定函数才能进入自定义函数，然而如果想在任意位置进入自定义函数（即不调用 syscall 进入函数），就需要为有这种需求的函数指定探针，然后在需求处加入节点（类似于 trap）运行到节点处进入自定义函数

  - c 代码

    ~~~c
    // main.c
    
    #include <uapi/linux/ptrace.h>
    #include <sys/sdt.h>
    
    int main(){
        DTRACE_PROBE(reg,user_probe_func) ;
        // process
        DTRACE_PROBE(reg,user_probe_func) ;
        //若 attach_probe 将 hello 与 user_probe_func 绑定，两次 DTRACE 都会进入 hello
    }
    
    int hello(struct pt_regs* ctx){
        // process
    }
    ~~~

    

  - py 代码

    ~~~python
    # 监控进程的 pid
    u = USDT(pid=int(pid))
    # 设立用户态探针
    u.enable_probe(probe="[user_probe_func]",fn_name="[loaded_fn_name]")
    # 插入内核，本质是自己定义了一个 probe，k probe 只能监控内核的调用
    b = BPF(src_file="[file_path]",usdt_contexts=[u])
    ~~~

    

- 输入输出

  - c 代码

    ~~~c
    bpf_trace_printk("[msg]") ;
    ~~~

  - py 代码

    ~~~python
    # 时刻监控 trace_pipe 文件，实时输出 msg 并阻塞
    b.trace_print()
    
    # 返回 task , pid , cpu , flags , ts , msg 元组 / 只阻塞一次
    (task , pid , cpu , flags , ts , msg) = b.trace_fields()
    ~~~

    一个 msg 只有一个 bpf_trace_printk 的信息，其工作原理可能是一个 msg 队列，一次 trace_fields 只能将头部指针向后挪动一位，trace_print() 由于阻塞，会一直输出直到队列为空

- 元素对象

  所有在 probe.c 文件中用宏创建的对象都可以用 `b["event"]` 来传递，包括哈希表、列表等

  

- 创建查找表

  ~~~c
  #include <uapi/linux/ptrace.h>
  
  // 创建 hash map ，默认 key 为 u64 类型
  BPF_HASH(last) ; 
  BPF_HASH(start, struct request*) ; // 以 struct request* 为 key
  
  // 查找元素 返回指向元素的指针
  ele_ptr = last.lookup(&key) ; // 查找失败为 NULL
  
  // 插入元素
  last.update(&key,&ele) ;
  
  // 删除 key
  last.delete(&key)
  ~~~

  **注意**：eBPF 会检查控制流，如果有任何可能导致解引用 NULL 现象，会拒绝载入内核，因此任何指针位置都需要特判空指针

- 记录时间

  ~~~c
  // 记录 ns 级时间 1 * 10^{-9} s 
  u64 time = bpf_ktime_get_ns() ;
  ~~~

- 建立管道

  - c 代码

    ~~~c
    #include <linux/sched.h>
    
    BPF_PER_OUTPUT(events) ; // 建立名为 events 管道
    
    int hello(struct pt_regs* ctx){
        struct data_t dat = {...} ;
        
        // 向管道提交信息，每次 perf_submit 往管道位置塞入一个信息 
        events.perf_submit(ctx,&dat,sizeof(dat)) ;
        
    }
    
    ~~~

  - py 代码

    ~~~python
    # 创建与管道交互的函数
    def trigger_event(cpu,data,size): 
        dat = b["events"].event(data) # data 实际上是管道中信息的编号，取出该位置信息
        # process dat
        return
    
    # 建立管道和函数的交互
    b["events"].open_perf_buffer(trigger_event)
    
    # 输出管道中所有的内容，若管道为空则阻塞
    b.perf_buffer_poll()
    ~~~

- 进程信息

  ~~~c
  // 获得 pid
  pid = bpf_get_current_pid_tgid() ;
  // 获得调用的系统函数名
  bpf_get_current_comm(&comm,sizeof(comm)) ;
  ~~~

- 可视化工具

  - c 代码

    ~~~c
    // 创建直方图
    BPF_HISTOGRAM(dist) ;
    
    int hello(void* ctx, struct request* req){
        // 将 dist 中 idx 索引指向值 +1
    	dist.increment(req->idx) ;
    }
    ~~~

  - py 代码

    ~~~python
    # 将直方图下标以 2 的次幂模式打印出来
    b["dist"].print_log2_hist("kbyte")
    # 清空直方图
    b["dist"].clear()
    ~~~

    