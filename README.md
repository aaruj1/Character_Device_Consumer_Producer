# Number Pipe- Producer Consumer Problem

This code creates a character device to replicate number pipe for producer consumer problem.
I have used 3 semaphores in the kernel code (character device)  namely, full, empty, and mutex to handle the locking of buffer during read and write operations.
I am also using a circular approach on read and write indices to maintain FIFO ordering.  The race and deadlock conditions are handled by using down_interruptible() and up() operations on 3 semaphores mentioned earlier.

## Below are the steps to compile and run the program :

### To compile and run number_pipe :
    Step 1: Use the makefile to compile the code
        $make
    Step 2: Load module using “insmod” with superuser privileges
        $sudo insmod number_pipe.ko buffer_length=<#No>

### To compile and run producer_numbers.c, consumer_numbers.c :

    Step 1: in terminal comiple the code producer_numbers.c
        $gcc -o producer_numbers producer_numbers.c
    Step 2: in onther terminal compile the code consumer_numbers.c
        $gcc -o consumer_numbers consumer_numbers.c
    
    Step 3: to run producer_numbers
        $sudo ./producer_numbers /dev/number_pipe
    Step 4: to run consumer_numbers
        $sudo ./consumer_numbers /dev/number_pipe
        
If any problem contact me at: 
Name : Antra Aruj
Email : aaruj1@binghamton.edu
