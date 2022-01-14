# Redis CRDT Experiment

Here are the experiments we do on CRDT-Redis. There are 3 folders here:

* *redis_test* : The bash scripts and server configuration files. 
  * **.sh* : Bash scripts for starting and testing on local machine, and bench program will run these scripts in remote.
* *bench* : The experiment code. Constructs the experiment, generates and runs operations, get logs and results.
  * *exp_env.h* : Construct the experiment environment. Start servers, construct replication, setup delays. Shutdown and cleanup when the experiment finishes.
  * *exp_runner.h* : Run the exp. Start client threads, calls the generator and records information from server to log.
  * *exp_setting.h* : The setting parameters of experiment.
  * *main.cpp* : The entry of the experiment.
  * *util.h* : Some utilities: random generator, redis client, redis reply. And the base classes of the CRDT experiments:
    * cmd : the CRDT command
    * generator : cmd generation logic
    * rdt_exp : experiment settings and experiment instance generation
  * *rpq* and *set* : Actual PQ and Set experiment folder. They extend the base classes in *util.h*, and implement the specific CRDT experiment logics.

## Experiment Framework

We run all server nodes in ECS and run client nodes on a dedicated machine. 

We start, replicate, and shutdown redis servers by local scripts. The scripts are called by ssh. 

The tarces will be stored in the *results* folder. 

## Perform the Experiment

To perform our experiments, follow the instructions below. First you need to compile the CRDT-Redis. You can modify the IP settings at *bench/exp_env.h*. Then start the experiment. 