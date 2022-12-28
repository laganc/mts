#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define BILLION  1000000000.0

// INITIALIZATION VARIABLES
int num_threads = 0;              // Number of train threads; global because used in multiple instances
int consecutive_west_trains = 0;  // Number of consecutive trains in the west direction
int consecutive_east_trains = 0;  // Number of consecutive trains in the east direction
int track_in_use = 0;             // If track is in use
int total_trains_crossed = 0;     // Tally up the amount of trains crossed to alert dispatch thread that all trains have crossed
char last_train = ' ';            // Last train direction
int threads_created = 0;          // Trains don't start loading until all train threads are created

int eastHigh_inUse = 0;           // If eastHigh station is currently being modified
int eastLow_inUse = 0;            // If eastLow station is currently being modified
int westHigh_inUse = 0;           // If westHigh station is currently being modified
int westLow_inUse = 0;            // If westLow station is currently being modified

// Struct and variables for getting runtime; global because used in multiple instances
struct timespec start, end;
int hours = 0;
int minutes = 0;
double seconds = 0;

// Mutex initialization for each station
pthread_mutex_t eastHigh_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t eastLow_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t westHigh_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t westLow_mutex = PTHREAD_MUTEX_INITIALIZER;

// Convars for each station
pthread_cond_t eastHigh_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t eastLow_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t westHigh_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t westLow_cond = PTHREAD_COND_INITIALIZER;

// Struct for trains
typedef struct train {
  int number;
  char direction;
  int loadTime;
  int crossTime;
  struct train* next;
} Train;

// Initialize train stations
Train* eastHigh = NULL;
Train* eastLow = NULL;
Train* westHigh = NULL;
Train* westLow = NULL;

/*
  Intiialize a new train
  Parameters:
    n = number/ID of train
    d = direction of train ('E', 'W', 'e', 'w')
    l = loading time of train
    c = crossing time of train
*/
Train* newTrain(int n, char d, int l, int c){
  Train* temp = (Train*)malloc(sizeof(Train));
  temp->number = n;
  temp->direction = d;
  temp->loadTime = l;
  temp->crossTime = c;
  temp->next = NULL;
  return temp;
}

/*
  Remove the first train from a station
  Parameters:
    station = train station
*/
void pop(Train** station){
  // If station is not empty, set head of station to next train
  if((*station)->next != NULL){
    (*station) = (*station)->next;
  }

  // If station is empty, set head of station to NULL
  else{
    (*station) = NULL;
  }
}

/*
  Add a train to a station depending on priority (load time)
  Parameters:
    data = data of train to be added to station
    station = train station
*/
void push(Train* data, Train** station){
  // Create new train from train data
  Train* new_train = newTrain(data->number, data->direction, data->loadTime, data->crossTime);

  // If station is empty, add new train to front of station
  if((*station) == NULL){
    (*station) = new_train;
    return;
  }

  // If front train's priority is lower than new train's priority, add new train to front of station
  if (((*station)->loadTime > new_train->loadTime) || ((*station)->loadTime == new_train->loadTime && (*station)->number > new_train->number)) {
    new_train->next = *station;
    (*station) = new_train;
  }

  // Otherwise, find the correct spot for the new train according to it's priority
  else{
    Train* temp = (*station);
    while (temp->next != NULL && temp->next->loadTime <= data->loadTime) {
      if(temp->next->loadTime == data->loadTime && temp->next->number > data->number){
        new_train->next = temp->next;
        temp->next = new_train;
        return;
      }
      temp = temp->next;
    }
    new_train->next = temp->next;
    temp->next = new_train;
  }
  return;
}

/*
  "Middleman" function to lock a station using mutex in order to add the train to the station
  Parameters:
    data = data of train to be added to station
*/
void addToStation(Train* data){
  // Depending on direction of train, add to respective station
  switch(data->direction){
    // If direction is eastbound and has high priority
    case 'E': {
      pthread_mutex_lock(&eastHigh_mutex);
      if(eastHigh_inUse != 0) pthread_cond_wait(&eastHigh_cond, &eastHigh_mutex);
      eastHigh_inUse++;

      push(data, &eastHigh);
      
      pthread_cond_signal(&eastHigh_cond); 
      pthread_mutex_unlock(&eastHigh_mutex);
      eastHigh_inUse = 0;

      pthread_exit(NULL);
      break;
    }
    // If direction is eastbound and has low priority
    case 'e': {
      pthread_mutex_lock(&eastLow_mutex);
      if(eastLow_inUse != 0) pthread_cond_wait(&eastLow_cond, &eastLow_mutex);
      eastLow_inUse++;

      push(data, &eastLow);

      pthread_cond_signal(&eastLow_cond); 
      pthread_mutex_unlock(&eastLow_mutex);
      eastLow_inUse = 0;

      pthread_exit(NULL);
      break;
    }
    // If direction is westbound and has high priority
    case 'W': {
      pthread_mutex_lock(&westHigh_mutex);
      if(westHigh_inUse != 0) pthread_cond_wait(&westHigh_cond, &westHigh_mutex);
      westHigh_inUse++;

      push(data, &westHigh);

      pthread_cond_signal(&westHigh_cond); 
      pthread_mutex_unlock(&westHigh_mutex);
      westHigh_inUse = 0;
      
      pthread_exit(NULL);
      break;
    }
    // If direction is westbound and has low priority
    case 'w': {
      pthread_mutex_lock(&westLow_mutex);
      if(westLow_inUse != 0) pthread_cond_wait(&westLow_cond, &westLow_mutex);
      westLow_inUse++;

      push(data, &westLow);

      pthread_cond_signal(&westLow_cond); 
      pthread_mutex_unlock(&westLow_mutex);
      westLow_inUse = 0;

      pthread_exit(NULL);
      break;
    }
  }
}

/*
  Get current runtime of the program. Note that this program already has a running start time.
  Parameters:
    str = buffer to store and return the formatted time
*/
char* get_runtime(char* str){
  clock_gettime(CLOCK_REALTIME, &end);
  double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / BILLION;
  hours = time_spent/3600;
  minutes = (time_spent - hours*3600)/60;
  seconds = (time_spent - minutes*60);

  sprintf(str, "%02d:%02d:%04.1f", hours, minutes, seconds);
  return str;
}

/*
  Method for trains to start loading
  Parameters:
    args = data about the current thread/train
*/
void* loadTrain(void* args){
  char str[12];
  char* loadtime;
  Train *data;
  data = (Train*) args;

  while(threads_created != num_threads);

  // Simulate the train loading
  usleep(data->loadTime*100000);

  // Get current runtime of program as load time
  loadtime = get_runtime(str);

  // If direction is eastbound
  if(data->direction == 'E' || data->direction == 'e') printf("%s Train %d is ready to go East\n", loadtime, data->number);
  // If direction is westbound
  else printf("%s Train %d is ready to go West\n", loadtime, data->number);

  // Add train to respective station when finished loading
  addToStation(data);
  return NULL;
}

/*
  Split up input lines by space
  Parameters:

*/
void tokenize(char* args[256], char* input){
  args[0]=strtok(input," \n");
  int i=0;
  
  while(args[i]!=NULL){
    args[i+1]=strtok(NULL," \n");
    i++;
  }
}

/*
  Print all trains in all stations, used for debugging purposes.
  Parameters: none
*/
void printStations(){
  printf("eastHigh stations in order:\n");
  while(eastHigh != NULL){
    printf("%d\n", eastHigh->number);
    eastHigh = eastHigh->next;
  }
  printf("\neastLow stations in order:\n");
  while(eastLow != NULL){
    printf("%d\n", eastLow->number);
    eastLow = eastLow->next;
  }
  printf("\nwestHigh stations in order:\n");
  while(westHigh != NULL){
    printf("%d\n", westHigh->number);
    westHigh = westHigh->next;
  }
  printf("\nwestLow stations in order:\n");
  while(westLow != NULL){
    printf("%d\n", westLow->number);
    westLow = westLow->next;
  }
}

/* 
  Method for trains to start crossing
  Parameters:
    direction = direction of train
*/
void crossTrain(char direction){
  // If direction is eastbound with high priority
  if(direction =='E'){
    // Variables for crossing times
    char str[12];
    char str1[12];
    char* crossStart;
    char* crossEnd;

    last_train = 'E'; // Set last train
    consecutive_east_trains++;  // Increment consecutive trains in direction
    track_in_use = 1;
    crossStart = get_runtime(str);  // Get time that train is going on main track

    printf("%s Train %d is ON the main track going East\n", crossStart, eastHigh->number);
    usleep(eastHigh->crossTime*100000);
    crossEnd = get_runtime(str1); // Get time that train is going off main track
    printf("%s Train %d is OFF the main track after going East\n", crossEnd, eastHigh->number);

    // Lock train station to remove the train
    pthread_mutex_lock(&eastHigh_mutex);
    if(eastHigh_inUse != 0) pthread_cond_wait(&eastHigh_cond, &eastHigh_mutex);
    eastHigh_inUse++;

    pop(&eastHigh);
    pthread_cond_signal(&eastHigh_cond); 
    pthread_mutex_unlock(&eastHigh_mutex);
    eastHigh_inUse = 0;
    track_in_use = 0;
  }
  // If direction is eastbound with low priority
  else if(direction =='e'){
    char str[12];
    char str1[12];
    char* crossStart;
    char* crossEnd;

    last_train = 'e';
    consecutive_east_trains++;
    track_in_use = 1;
    crossStart = get_runtime(str);
    printf("%s Train %d is ON the main track going East\n", crossStart, eastLow->number);
    usleep(eastLow->crossTime*100000);
    crossEnd = get_runtime(str1);
    printf("%s Train %d is OFF the main track after going East\n", crossEnd, eastLow->number);

    pthread_mutex_lock(&eastLow_mutex);
    if(eastLow_inUse != 0) pthread_cond_wait(&eastLow_cond, &eastLow_mutex);
    eastLow_inUse++;

    pop(&eastLow);
    pthread_cond_signal(&eastLow_cond); 
    pthread_mutex_unlock(&eastLow_mutex);
    eastLow_inUse = 0;
    track_in_use = 0;
  }
  // If direction is westbound with high priority
  else if(direction == 'W'){
    char str[12];
    char str1[12];
    char* crossStart;
    char* crossEnd;

    last_train = 'W';
    consecutive_west_trains++;
    track_in_use = 1;
    crossStart = get_runtime(str);
    printf("%s Train %d is ON the main track going West\n", crossStart, westHigh->number);
    usleep(westHigh->crossTime*100000);
    crossEnd = get_runtime(str1);
    printf("%s Train %d is OFF the main track after going West\n", crossEnd, westHigh->number);

    pthread_mutex_lock(&westHigh_mutex);
    if(westHigh_inUse != 0) pthread_cond_wait(&westHigh_cond, &westHigh_mutex);
    westHigh_inUse++;

    pop(&westHigh);
    pthread_cond_signal(&westHigh_cond); 
    pthread_mutex_unlock(&westHigh_mutex);
    westHigh_inUse = 0;
    track_in_use = 0;
  }
  // If direction is westbound with low priority
  else if(direction =='w'){
    char str[12];
    char str1[12];
    char* crossStart;
    char* crossEnd;

    last_train = 'w';
    consecutive_west_trains++;
    track_in_use = 1;
    crossStart = get_runtime(str);
    printf("%s Train %d is ON the main track going West\n", crossStart, westLow->number);
    usleep(westLow->crossTime*100000);
    crossEnd = get_runtime(str1);
    printf("%s Train %d is OFF the main track after going West\n", crossEnd, westLow->number);


    pthread_mutex_lock(&westLow_mutex);
    if(westLow_inUse != 0) pthread_cond_wait(&westLow_cond, &westLow_mutex);
    westLow_inUse++;

    pop(&westLow);
    pthread_cond_signal(&westLow_cond); 
    pthread_mutex_unlock(&westLow_mutex);
    westLow_inUse = 0;
    track_in_use = 0;
  }
  return;
}

/*
  Dispatch thread to indicate trains to leave their station if it's their turn
  Parameters: None
*/
void* dispatchTrains(void* args){
  while(1){
    usleep(100);
    // If eastHigh has a train and track isn't in use
    if(eastHigh != NULL && track_in_use == 0){
      // If westHigh has a train and last train across the track was eastbound, then let westHigh train go first
      if(westHigh != NULL && (last_train == 'e' || last_train == 'E')){  
        consecutive_east_trains = 0;
        crossTrain('W');
      }
      // If westLow has a train and there have been 4+ eastbound trains in a row, allow westLow to go first
      else if(westLow != NULL && consecutive_east_trains >= 4){
        consecutive_east_trains = 0;
        crossTrain('w');
      }
      // If none of the above conditions are true, allow eastHigh to cross
      else{
        consecutive_west_trains = 0;
        crossTrain('E');
      }
      total_trains_crossed++;
    }

    // If westHigh has a train and track isn't in use
    else if(westHigh != NULL && track_in_use == 0){
      // If eastHigh has a train and last train across the track was westbound, then let eastHigh train go first
      // Moreover, if no train has crossed yet, if eastHigh has a train, let eastHigh go first
      if(eastHigh != NULL && (last_train == 'w' || last_train == 'W' || last_train == ' ')){
        consecutive_west_trains = 0;
        crossTrain('E');
      }
      // If eastLow has a train and there have been 4+ westbound trains in a row, allow eastLow to go first
      else if(eastLow != NULL && consecutive_west_trains >= 4){
        consecutive_west_trains = 0;
        crossTrain('e');
      }
      // If none of the above conditions are true, allow westHigh to cross
      else{
        consecutive_east_trains = 0;
        crossTrain('W');
      }
      total_trains_crossed++;
    }

    // If eastLow has a train and track isn't in use
    else if(eastLow != NULL && eastHigh == NULL && westHigh == NULL && track_in_use == 0){
      // If westLow has a train and last train across the track was eastbound, then let westLow train go first
      if(westLow != NULL && (last_train == 'e' || last_train == 'E')){
        consecutive_east_trains = 0;
        crossTrain('w');
      }
      // If above condition is not true, allow eastLow to cross
      else{
        consecutive_west_trains = 0;
        crossTrain('e');
      }
      total_trains_crossed++;
    }

    // If westLow has a train and track isn't in use
    else if(westLow != NULL && eastHigh == NULL && westHigh == NULL && track_in_use == 0){
      // If eastLow has a train and last train across the track was westbound, then let eastLow train go first
      // Moreover, if no train has crossed yet, if eastHigh has a train, let eastHigh go first
      if(eastLow != NULL && (last_train == 'w' || last_train == 'W' || last_train == ' ')){
        consecutive_west_trains = 0;
        crossTrain('e');
      }
      // If above condition is not true, allow westLow to cross
      else{
        consecutive_east_trains = 0;
        crossTrain('w');
      }
      total_trains_crossed++;
    }
    if(total_trains_crossed == num_threads) break;
  }
  // Exit dispatch thread when all trains have been processed.
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

  char s[10];
  char c[10];
	char* train_info[256];
  int rc;
  int rc1;
  void* status;

  // Set thread attributes
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  // Open input file
  FILE *fp;
  fp = fopen(argv[1], "r");

  // If input file is incorrect, return an error
  if(fp == NULL){
    printf("File not found.\n");   
    exit(1);             
  }

  // Increment num_threads according to how many lines there are in the input file
  while(!feof(fp)){
    fgets(c, 10, fp);
    num_threads++;
  }

  //printf("%s",c);

  Train train_array[num_threads];
  rewind(fp);

  // Create number of threads needed
  pthread_t dispatch;
  pthread_t thread[num_threads];

  // Create dispatch thread
  rc = pthread_create(&dispatch, &attr, dispatchTrains, NULL);
  if (rc) {
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
  }
  
  // Create train threads
  for(long i = 0; i < num_threads; i++){
    fgets(s,16, fp);
    tokenize(train_info, s);

    // Set train info to train_array to send to thread method
    train_array[i].number = i;
    train_array[i].direction = *train_info[0];
    train_array[i].loadTime = atoi(train_info[1]);
    train_array[i].crossTime = atoi(train_info[2]);

    rc = pthread_create(&thread[i], &attr, loadTrain, (void*) &train_array[i]);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
    threads_created++;
  }

  // Start runtime
  clock_gettime(CLOCK_REALTIME, &start);

  fclose(fp);

  // Destroy attribute
  pthread_attr_destroy(&attr);
  // Join train threads
  for(int t=0; t<num_threads; t++) {
    rc = pthread_join(thread[t], &status);
    if (rc) {
      printf("ERROR; return code from pthread_join() is %d\n", rc);
      exit(-1);
    }
  }

  // Join dispatch thread
  rc = pthread_join(dispatch, &status);
  if (rc) {
    printf("ERROR; return code from pthread_join() is %d\n", rc);
    exit(-1);
  }

  // Destroy all mutexes and condition variables
  pthread_mutex_destroy(&eastHigh_mutex);
  pthread_mutex_destroy(&eastLow_mutex);
  pthread_mutex_destroy(&westHigh_mutex);
  pthread_mutex_destroy(&westLow_mutex);
  pthread_cond_destroy(&eastHigh_cond);
  pthread_cond_destroy(&eastLow_cond);
  pthread_cond_destroy(&westHigh_cond);
  pthread_cond_destroy(&westLow_cond);

  pthread_exit(NULL);
}