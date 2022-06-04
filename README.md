# Paisley-Park



_佩斯利公园，可以引导人们去到该去的地方。有时候呈现为手机上的网络地图，有时候会呈现为某种印记，绝不能看漏了_



#### Ubuntu

- see doc



#### Feature

- please use two terminals

- terminal 1

  ~~~shell
  cd src
  make clean
  make build
  sudo python3 bpf_run.py
  # hint: when io_call finish, use CTRL+C to get the results
  ~~~

- terminal 2

  ~~~shell
  ./io_call
  ~~~

  