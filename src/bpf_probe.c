#include <uapi/linux/ptrace.h>


// round time1 time2 ...
BPF_HASH(io_uring_submit_call_map) ;
BPF_HASH(io_uring_submit_ret_map) ;



int trace_io_uring_submit_call( struct pt_regs *ctx ){
    // provide the call absolute call time for submit
    u64 round = 0 , key = 0 , time  ;
    u64 *val ;

    time = bpf_ktime_get_ns() ; // call time
    val = io_uring_submit_call_map.lookup(&key) ; // search round

    if (val==NULL){ // the first round
        round = 1 ;
        io_uring_submit_call_map.insert(&key,&round) ;
        io_uring_submit_call_map.insert(&round,&time) ;
    }else{
        round = *val + 1 ;
        io_uring_submit_call_map.delete(&key) ;
        io_uring_submit_call_map.insert(&key,&round) ;
        io_uring_submit_call_map.insert(&round,&time) ;
    }
    
    return 0 ;

}

int trace_io_uring_submit_ret( struct pt_regs *ctx ){
    // provide the call absolute call time for submit
    u64 round = 0 , key = 0 , time  ;
    u64 *val ;

    time = bpf_ktime_get_ns() ; // call time
    val = io_uring_submit_ret_map.lookup(&key) ; // search round

    if (val==NULL){ // the first round
        round = 1 ;
        io_uring_submit_ret_map.insert(&key,&round) ;
        io_uring_submit_ret_map.insert(&round,&time) ;
    }else{
        round = *val + 1 ;
        io_uring_submit_ret_map.delete(&key) ;
        io_uring_submit_ret_map.insert(&key,&round) ;
        io_uring_submit_ret_map.insert(&round,&time) ;
    }
    
    return 0 ;

}


BPF_HASH(blockio_call_map) ;
BPF_HASH(blockio_ret_map) ;



int trace_blockio_call( struct pt_regs *ctx ){
    // provide the call absolute call time for submit
    u64 round = 0 , key = 0 , time  ;
    u64 *val ;

    time = bpf_ktime_get_ns() ; // call time
    val = blockio_call_map.lookup(&key) ; // search round

    if (val==NULL){ // the first round
        round = 1 ;
        blockio_call_map.insert(&key,&round) ;
        blockio_call_map.insert(&round,&time) ;
    }else{
        round = *val + 1 ;
        blockio_call_map.delete(&key) ;
        blockio_call_map.insert(&key,&round) ;
        blockio_call_map.insert(&round,&time) ;
    }
    
    return 0 ;

}

int trace_blockio_ret( struct pt_regs *ctx ){
    // provide the call absolute call time for submit
    u64 round = 0 , key = 0 , time  ;
    u64 *val ;

    time = bpf_ktime_get_ns() ; // call time
    val = blockio_ret_map.lookup(&key) ; // search round

    if (val==NULL){ // the first round
        round = 1 ;
        blockio_ret_map.insert(&key,&round) ;
        blockio_ret_map.insert(&round,&time) ;
    }else{
        round = *val + 1 ;
        blockio_ret_map.delete(&key) ;
        blockio_ret_map.insert(&key,&round) ;
        blockio_ret_map.insert(&round,&time) ;
    }
    
    return 0 ;

}
