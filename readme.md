# Multi-Threaded Scheduling Program

## Description
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

## How to use
In order to use this program, you must first invoke the makefile. To do so,
while you are in the directory in the command terminal, simply type and enter "make".
This will create the executable "mts" file. Once that is done, you can use the
command "./mts input.txt" to run the program. Note that "input.txt" does not
necessarily have to be named that, however it must be a ".txt" or ".in" file with
the format specified above.
