-   problems in traditional multithreaded applications

    -   Those applications often suffer from contention issues when multiple 
        threads try to access shared resources concurrently. 

    -   Locking mechanisms, such as mutexes or semaphores, are commonly used to 
        synchronize access to shared data structures, but they can introduce 
        significant overhead and contention, leading to poor scalability and 
        increased latency. 

    -   However, context switches are costly as the cache associated with the 
        previous thread or process may be invalidated, causing cache misses. 
        
    -   The cache might also need to be flushed, resulting in additional 
        latency. 

    -   Finally when a new thread or process starts, it takes time to populate 
        the cache with relevant data, affecting preformance initially. 

-   Compare And Swap (CAS) operations

    -   Processes

        -   Firstly, each thread involved in the CAS operation reads the 
            current value of the shared variable from memory. 

        -   Next, the thread compares this current value with its expected 
            value, which it has in mind. If the current value matches the 
            expected value, indicating that no other thread has modified the 
            shared variable since the read operation, the thread proceeds to 
            atomically swap the current value with a new desired value. 

        -    Following the swap, the CAS operation returns a status indicating 
            whether the swap was successful or not. If successful, it signifies 
            that the thread has successfully updated the shared variable; 
            otherwise, it means that another thread had modified the shared 
            variable in the meantime, requiring the thread to retry the CAS 
            operation.

    -   Cons of CAS

        CAS is definitely better than the use of locks, but they’re still not cost-free.

        -   Firstly, orchestrating a complex system using CAS operations can 
            be harder than the use of locks.

        -   Secondly, to ensure atomicity, the processor locks its instruction 
            pipeline, and a memory barrier is used to ensure that changes made 
            by a thread become visible to other threads.

        -   Queues can be implemented using linked lists or arrays, but 
            unbounded queues can lead to memory exhaustion if producers outpace 
            consumers. To prevent this, queues are often bounded or actively 
            tracked in size. 

        -   However bounded queues introduce write contention on head, tail, 
            and size variables, leading to high contention and potential cache 
            coherence issues. Managing concurrent access to queues becomes 
            complex, especially when dealing with multiple producers and consumers.

-   What does disruptor resolve

    The LMAX Disruptor aims to resolve the mentioned issues and optimise memory 
    allocation efficiency while operating effectively on modern hardware.

    Achivments:

    -   LMax state that the Disruptor achieves significantly lower mean latency 
        by three orders of magnitude, compared to an equivalent queue-based 
        approach in a three-stage pipeline.

    -   Additionally, the disruptor demonstrates approximately 8 times higher 
        throughput handling capacity

    How does disruptor achieve that?

    -   It achieves this through a pre-allocated bounded data structure called 
        a ring buffer. Producers add data to the ring buffer, and one or more 
        consumers process it. 

    -   The ring buffer can store either an array of pointers to entries or an 
        array of structures representing the entries. Typically, each entry in the 
        ring buffer does not directly contain the data being passed but serves 
        as a container or holder for the data. 

    -   Producer

        In most use cases of the Disruptor, there is typically only one 
        producer, such as a file reader or network listener, resulting in no 
        contention on sequence/entry allocation.

        However, in scenarios with multiple producers, they race to claim the 
        next available entry in the ring buffer, which can be managed using a 
        CAS operation on the sequence number. 

    -   Consumer

        Once a producer copies the relevant data to the claimed entry, it can 
        make it public to consumers by committing the sequence.

        Consumers wait for a sequence to become available before reading the 
        entry, and various strategies can be employed for waiting, including 
        condition variables within a lock or looping with or without thread 
        yield.

        The Disruptor avoids CAS contention present in lock-free 
        multi-producer-multi-consumer queues, making it a scalable solution.

    -   Sequencing

        Sequencing is a fundamental concept in the Disruptor for managing 
        concurrency. Producers and consumers interact with the ring buffer 
        based on sequencing.

        Producers claim the next available slot in the sequence, which can be 
        a simple counter or an atomic counter with CAS operations.

        Once a slot is claimed, the producer can write to it and update a 
        cursor representing the latest entry available to consumers. 

        Consumers wait for a specific sequence by using memory barriers to read 
        the cursor, ensuring visibility of changes.

        Consumers maintain their own sequence to track their progress and 
        coordinate work on entries. In cases with a single producer, locks or 
        CAS operations are not needed, and concurrency coordination relies 
        solely on memory barriers. 

        The Disruptor offers an advantage over queues when consumers wait for 
        an advancing cursor in the ring buffer. If a consumer notices that the 
        cursor has advanced multiple steps since it last checked, it can 
        process entries up to that sequence without involving concurrency 
        mechanisms. 

        This allows the consumer to quickly catch up with producers during 
        bursts, balancing the system. This batching approach improves 
        throughput, reduces latency, and provides consistent latency regardless 
        of 