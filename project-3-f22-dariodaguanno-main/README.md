# Project-3-F22
# Project 3 Exploring Synchronization Among Processes and Threads

## The Problem:
You are given three (3) processes in one program that work together to solve a producer consumer problem. Two Processes are “producers”, each producing its own type of product in a continuous loop. That is, one product type produced by one producer and a different product type by the other producer.

One Process is a “consumer” of products. The consumer process consists of five (5) threads.
•	One thread is a ‘distributor’ thread
•	Two (2) threads consume one type of product. That is, each of these two threads consumes only product type 1.
•	Two (2) threads consume a second type of product. That is, each of these two threads consumes only product type 2.

The consumer process contains two (2) product storage buffers, each comprised of a fixed number of slots. The number of slots in the buffers are to be different (one has more slots than the other). You choose and specify the number of slots in each buffer as a definition in your program solution

Communication between the producer processes and the consumer process is to be through a single “pipe”. This single, shared pipe is used to communicate between each producer process and the consumer process. Each producer process writes into this pipe the product to be consumed by the consumer process.

## Action of the Producer Processes
A producer process executes in a loop until the program is ‘killed’ from the command line. 
In each iteration of the loop, the producer sends one of its products to the consumer process via the shared pipe. After sending the product, the producer delays for a variable amount of time (randomly chosen between 0.01 seconds and 0.2 seconds) and then continues to the next iteration of its production loop. 

The data sent into the pipe for each production is to include two integers: a ‘count’ and the product number (product_1 or product _2, depending on which producer is sending). Each producer will increment a counter as it produces an item. The value of the counter is the current production number for that product type. The production number for the item (‘count’ value) is sent along with the product type into the pipe.

## Action of the Consumer Process and threads
The consumer process continues to execute until the program is ‘killed’ from the command line. 

The ‘distributor’ consumer thread is used to receive products from both producer processes and places them in the buffer that is specific for the type of product it received. Each buffer element is to contain the production number (‘count’ value) received from the producer. 

Each of the ‘product consumer threads’ executes in a loop removing products from its designated product buffer. As a product is removed from the buffer, a count of the consumptions is to be incremented for that product type. Note: two threads share the count of consumptions for each product type. After consuming the product and updating the ‘consumption sequence number’, the product consumer thread will write the transaction data to a file as described below. The product consumer thread will then loop back to look for another of its product type to consume.

## Synchronization
This project includes features of the consumer-producer problem that we have discussed in class. There are different approaches that one can take to implement the necessary synchronization. But, for this project, you are to use condition variables to achieve synchronization and locks to provide the associated mutual exclusion for all concurrent accesses.

Access to each buffer in the consumer process is to be shared among the associated two product consumer threads and the distributor thread. The access must be done safely with no race conditions.

## Program Output
Products that are consumed by the product consumer threads are to be logged to a file. The file is to be your design, but contains data as follows. Each product consumer thread writes to the file: the product type, the consumption thread ID, the production sequence number (producer ‘count’ value) and the consumption sequence number (the counter value for consumption of that product item). These values can help you evaluate whether you are losing data and the way that your program is producing and consuming data. 

## Suggestions and Hints:
A single executable can be developed for a producer process with control arguments that specify any differences between producers.
A single product consumer thread function can be developed that includes parameters that specify any differences among the product consumer threads.
Consider how file descriptors are part of the PCB and are copied to child processes.
You should design the output file records so that you can use them for debugging as well as program output.


## Deliverables:
The first weekly deliverable includes: the creation of all processes and the pipe communication among processes; the design of the two consumer storage buffers; read and write (get/put) functions for the two buffers, and the product distributor thread.

Final program delivery: completion of the product consumer threads, output file design and write functions; sample data runs with output files

