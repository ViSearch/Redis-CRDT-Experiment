# Redis CRDT Experiment

This tutorial introduces how to reproduce our experiments on CRDT-Redis, and generate histories. 

[CRDT-Redis](https://github.com/elem-azar-unis/CRDT-Redis) is an open-source project on Github which revises the Redis data type store into a multi-master architecture and supports CRDT-based conflict resolution for concurrent updates on multiple master nodes.

## Setup CRDT-Redis

As we modified some code of the original CRDT-Redis for our experiments, we provide the complete code of CRDT-Redis in folder *redis-6.0.5* with the consent of the author. 

Please build CRDT-Redis in the default mode in at least 3 machines:

```bash
cd redis-6.0.5
make
sudo make install
```

## Configure Bench

Before experiment, you need to configure the IPs and ports for each machine running CRDT-Redis, in folder *experiment/bench/exp_env.h* . 

```cpp
class exp_env
{
	private:
		string available_hosts[3] = {"172.24.81.1", "172.24.81.2", "172.24.81.3"};
		string available_ports[5] = {"6379", "6380", "6381", "6382", "6383"};
```

## **Build & Run Bench**

The code of bench is in folder *experiment/bench*.

```
$ cd experiment/bench
$ make
$ ./bench_start [set|rpq] [o|r|rwf] [number] [sudopassword]
```

You can choose the data type *set* or *rpq* (redis priority queue). 

You can also choose the conflict resolution, o referring to add-win, r referring to remove-win ,and rwf referring to RWF-Framework. 

You have to select how many histories to be generated, and give a sudo-password to run redis in remote machines. 

For example, 

```
$ ./bench_start set r 1000 password123
```
