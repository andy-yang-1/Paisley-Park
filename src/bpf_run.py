from bcc import BPF,USDT
from time import sleep
import os

def get_call_time(call_map,ret_map):

    call_list = []
    ret_list = []

    for _ , v in call_map.items():
        call_list.append(v.value)

    for _ , v in ret_map.items():
        ret_list.append(v.value)

    call_list.sort()
    ret_list.sort()

    time_sum = 0.0

    size = 512 * 32 # iteration * batch

    for i in range(len(call_list)):
        time_sum += ret_list[i] - call_list[i] 

    time_sum /= size 

    return time_sum


# wait pid file
while(os.path.exists("pid_file")==False):
    pass

# read pid_file to know io_call process's pids
pid = int(open("pid_file","r").readline())
print("getpid:",pid)

# create user static probe
u = USDT(pid=pid) 

u.enable_probe(probe="blockio_call", fn_name="trace_blockio_call")
u.enable_probe(probe="blockio_ret", fn_name="trace_blockio_ret")

# load bpf
b = BPF(src_file="bpf_probe.c",usdt_contexts=[u]) 

b.attach_uprobe(name="uring",sym="io_uring_submit",fn_name="trace_io_uring_submit_call")
b.attach_uretprobe(name="uring",sym="io_uring_submit",fn_name="trace_io_uring_submit_ret") 


# b.trace_print()

try:
    sleep(9999999)
except KeyboardInterrupt:
    pass



# call_map = b["io_uring_submit_call_map"]
# ret_map = b["io_uring_submit_ret_map"]

# for i , v in call_map.items():
#     print(v.value)


# print(len(call_map.items()))

print("liburing average call time:",get_call_time(b["io_uring_submit_call_map"], b["io_uring_submit_ret_map"]))
print("blockio average call time:",get_call_time(b["blockio_call_map"], b["blockio_ret_map"]))

# print("finish",call_map)
