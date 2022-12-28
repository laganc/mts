Multi-Threaded Scheduling Program in C



DESCRIPTION OF PROGRAM
This program implements multi-threading in order to simulate trains loading,
being added to station queues, and crossing a single train track. All this
happens simultaneously, meaning every train loads at the same time, and
depending on priority, get dispatched from the train station as well.
Note that only one train is allowed on the train track at a time.

The "trains" we speak of are created via an input file. The program reads lines
from the file, and creates train "objects" which are used to simulate actual
trains loading and crossing. 

Each line of the input file must contain 3 items separated by spaces. 

First, a direction. This can be 'E', 'W', 'e', 'w' where 'E' and 'e' are East, 
while 'W' and 'w' are West. An uppercase letter means it has higher priority over
a lowercase letter.

Second, a load time. This must be an integer between 0 and 99. Note that this
does not mean between 0 and 99 seconds, it means between 0 and 9.9 seconds. This
time is measured in 10ths of a second.

Third, a crossing time. This must be an integer between 0 and 99. Note that this
does not mean between 0 and 99 seconds, it means between 0 and 9.9 seconds. This
time is measured in 10ths of a second.

Note that the last line of the input file must be information about the last train.



IMPORTANT FUNCTIONS USED IN THE PROGRAM
In order to read the input file, I utilized functions such as fopen(), feof(),
fgets(), strtok(), fclose(), and atoi(), as specified in the p2 outline. 

I also utilized POSIX threads from the <pthread.h> library available in C in
order to complete this program. From the library I used multiple functions in
order to create and exit threads, such as pthread_create(), pthread_exit(), and
pthread_join(). 

To make sure that threads aren't in conflict, I utilized mutexes and condition
variables using functions such as pthread_mutex_lock(), pthread_mutex_unlock(),
pthread_cond_wait(), and pthread_cond_signal(). I did not need to use the mutex
and condition variable initialization functions because I used the default
initializers for them: PTHREAD_MUTEX_INITIALIZER and PTHREAD_COND_INITIALIZER.

In order to get the runtime of the program at any given time, I used the
<time.h> library and utilized the clock_gettime() function. This was needed
for getting the time when a train has finished loading, is ON the track, and
subsequently, is OFF the track.



HOW TO USE THE PROGRAM
In order to use this program, you must first invoke the makefile. To do so,
while you are in the "p2" directory in the command terminal, simply enter "make".
This will create the executable "mts" file. Once that is done, you can use the
command "./mts input.txt" to run the program. Note that "input.txt" does not
necessarily have to be named that, however it must be a ".txt" or ".in" file with
the format specified above.



NOTE
My actual program is quite different from p2a. I will admit that I learned a lot
about how threads, mutexes, and convars work when actually programming with them,
therefore my program will not look very similar to what I specified in p2a.



CREDITS
Priority queue help: https://www.geeksforgeeks.org/priority-queue-using-linked-list/
POSIX threads help: https://hpc-tutorials.llnl.gov/posix/

