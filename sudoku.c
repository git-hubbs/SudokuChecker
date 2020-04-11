//**********************************************************
// Author: Jason Hubbs                                    //
// Date: 04-10-2020                                       //
// File: sudoku.c                                         //
// Description: Takes an input_file.txt as a parameter    //
//              and determines if the input is a valid    //
//              sudkou solution. Using the -f option      //
//              makes the program use child processes,    //
//              otherwise worker threads are used.        //
//**********************************************************
#include <stdio.h>
#include <signal.h>
#include <unistd.h> 
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/mman.h>

//SharedResult is contained in memory available to all child
//processes and contains the result variable.
typedef struct SharedResult {
  bool isValid;
  int parentPID;
}SharedResult;

//ThreadInfo contains thread information, and a copy is sent
//to each thread with specific information.
typedef struct ThreadInfo {
  int index;
  int sudoku_array[9][9];
  SharedResult *shared_memory;
} ThreadInfo;

//Function definitions
void parse_args(int argc, char *argv[]);
void print2dArray(const int matrix[9][9]); //Currently unused
void StartProcesses(ThreadInfo info);
void *StartThreads(ThreadInfo info);
void *CheckRow(void *info);
void *CheckCol(void *info);
void *CheckBox(void *info);
void CheckRowFork(ThreadInfo info);
void CheckColFork(ThreadInfo info);
void CheckBoxFork(ThreadInfo info);

/* These are the only two global variables allowed in your program */
static int verbose = 0;
static int use_fork = 0;

//**********************************************************
// Function: int main(int argc, char *argv[])             //
// Description: Main driver.                              //
//**********************************************************
int main(int argc, char *argv[])
{
  //Set memory spot accessable by all child processes
  SharedResult *shared_result_pointer = mmap(NULL, _SC_PAGESIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  shared_result_pointer->isValid = true;
  shared_result_pointer->parentPID = getpid();
  ThreadInfo info = {.shared_memory = shared_result_pointer};

  //Variables for reading input file
  FILE * file;
  char * line;
  size_t len = 0;
  ssize_t read;
  file = fopen(argv[1], "r");
 
  //Check for file
  if (file == NULL){
    printf("Error, input file not present.");
    exit(EXIT_FAILURE);
  }

  //Read file line by line
  int lineAccumulator = 0;
  int accumulator = 0;
  while ((read = getline(&line, &len, file) != -1)) {
    if(line[0] == 10) //Skip line if newline character
      getline(&line, &len, file);
    accumulator = 0;
    //Populate matrix after retrieving each line
    for(int i = 0; i < strlen(line); i++){
      char rawChar = line[i];
      int char2Num = rawChar - '0';
      if(rawChar != ' '){ //Skip whitespace
	info.sudoku_array[lineAccumulator][accumulator] = char2Num;
	accumulator++;
      }
    }
    lineAccumulator++;
  }  
  fclose(file);
  
  parse_args(argc, argv);
  //Print out information based on mode chosen via arguments and start
  //either thread function or process function.
  if (use_fork) {
    printf("We are forking child processes as workers.\n");
    StartProcesses(info);
  }
  else {
    printf("We are using worker threads.\n");
    StartThreads(info);
  }

  //Print out message beased on isValid switch. Limits message to main
  //thread and parent process.
  if(getpid() == info.shared_memory->parentPID){
    if(info.shared_memory->isValid != false)
      printf("The input is a valid Sudoku.\n");     
    else
      printf("The input is not a valid Sudoku.\n");    
  }
}

//**********************************************************
// Function: void parse_args(int argc, char *argv[])      //
// Description: Parses input arguments to determine which //
//              mode the program should run on.           //
//**********************************************************
void parse_args(int argc, char *argv[]){
  int c;
  while (1)
    {
      static struct option long_options[] =
        {
	  {"verbose", no_argument,       0, 'v'},
	  {"fork",    no_argument,       0, 'f'},
	  {0, 0, 0, 0}
        };
      int option_index = 0;
      c = getopt_long (argc, argv, "vf", long_options, &option_index);
      if (c == -1) break;
      
      switch (c)
        {
	case 'f':
	  use_fork = 1;
	  break;
	case 'v':
	  verbose = 1;
	  break;
	default:
	  exit(1);
        }
    }
}

//**********************************************************
// Function: void* StartThreads(ThreadInfo info)          //
// Description: Initializes and creates threads in thread //
//              create mode.                              //
//**********************************************************
void* StartThreads(ThreadInfo info){
  //Initializes threads and temporary info variables
  pthread_t rowThread[9];
  pthread_t colThread[9];
  pthread_t boxThread[9];
  ThreadInfo temp0[9] = {info, info, info, info, info, info, info, info, info};
  ThreadInfo temp1[9] = {info, info, info, info, info, info, info, info, info};
  ThreadInfo temp2[9] = {info, info, info, info, info, info, info, info, info};

  //Iteratively creates threads threads for rows, cols, and boxes in that order
  for(int i = 0; i <= 8; i++){
    temp0[i].index = i;
    pthread_create(&rowThread[i], NULL, CheckRow, &temp0[i]);
  }
  for(int i = 0; i <= 8; i++){
    temp1[i].index = i;
    pthread_create(&colThread[i], NULL, CheckCol, &temp1[i]);
  }
  for(int i = 0; i <= 8; i++){
    temp2[i].index = i;
    pthread_create(&boxThread[i], NULL, CheckBox, &temp2[i]);
  }

  //Joins threads in order of creation for simplicity
  for(int i = 0; i <= 8; i++)
    pthread_join(rowThread[i], NULL);
  for(int i = 0; i <= 8; i++)
    pthread_join(colThread[i], NULL);
  for(int i = 0; i <= 8; i++)
    pthread_join(boxThread[i], NULL);
}

//**********************************************************
// Function: void StartProcesses(ThreadInfo info)         //
// Description: Initializes and creates processes in      //
//              process create mode.                      //
//**********************************************************
void StartProcesses(ThreadInfo info){
  info.shared_memory->parentPID = getpid();
  ThreadInfo temp = info;
  int parent = 1;
  for(int i = 0; i < 9 && parent; i++){
    parent = fork();
    if(parent == 0){
      temp.index = i;
      CheckRowFork(temp);
    }
  }

  for(int i = 0; i < 9 && parent; i++){
    parent = fork();
    if(parent == 0){
      temp.index = i;
      CheckColFork(temp);
    }
  }
  for(int i = 0; i < 9 && parent; i++){
    parent = fork();
    if(parent == 0){
      temp.index = i;
      CheckBoxFork(temp);
    }
  }
  //while((info.parentPID = wait(0)) >1);
}

//**********************************************************
// Function: void* CheckRow(void *info)                   //
// Description: Checks the specific row as a worker       //
//              thread and updates isValid as needed.     //
//**********************************************************
void *CheckRow(void *info){
  //Typecast from void to ThreadInfo
  ThreadInfo *info0 = info;
  bool cF[9] = {false, false, false, false, false, false, false, false, false};
  for(int i = 0; i < 9; i++){
    int index = info0->sudoku_array[i][info0->index]-1;
    cF[index] = true;
  }  
  if((cF[0] == false)| (cF[1] == false)|
     (cF[2] == false)| (cF[3] == false)| (cF[4] == false)|
     (cF[5] == false)| (cF[6] == false)| (cF[7] == false)|
     (cF[8] == false)){
    info0->shared_memory->isValid = false;
    for(int j = 0; j < 9; j++){
      if(cF[j] == false){
	printf("Col %d doesn't have the required values. \n", info0->index+1);	
	break;
      }
    }
  } 
}

//**********************************************************
// Function: void* CheckCol(void *info)                   //
// Description: Checks the specific col as a worker       //
//              thread and update isValid as needed.      //
//**********************************************************
void *CheckCol(void *info){
  //Typecast from void to ThreadInfo
  ThreadInfo *info0 = info;
  bool cF[9] = {false, false, false, false, false, false, false, false, false};
  for(int i = 0; i < 9; i++){
    int index = info0->sudoku_array[info0->index][i]-1;
    cF[index] = true;
  }
  if((cF[0] == false)| (cF[1] == false)|
     (cF[2] == false)| (cF[3] == false)| (cF[4] == false)|
     (cF[5] == false)| (cF[6] == false)| (cF[7] == false)|
     (cF[8] == false)){
    info0->shared_memory->isValid = false;
    for(int j = 0; j < 9; j++){
      if(cF[j] == false){
	printf("Row %d doesn't have the required values. \n", info0->index+1);	
	break;
      }
    }
  }     
}

//***********************************************************
// Function: void* CheckBox(void *info)                    //
// Description: Checks the specific box as a worker thread //
//              and update isValid as needed.              //
//              This code probably should be shortened.    //
//***********************************************************
void *CheckBox(void *info){
  ThreadInfo *info0 = info;
  bool cF[9] = {false, false, false, false, false, false, false, false, false};
  if(info0->index == 0){
    for(int i = 0; i <= 2; i++){
      for(int j = 0; j <= 2; j++){
	int index = info0->sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info0->index == 1){
    for(int i = 0; i <= 2; i++){
      for(int j = 3; j <= 5; j++){
	int index = info0->sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info0->index == 1){
    for(int i = 0; i <= 2; i++){
      for(int j = 3; j <= 5; j++){
	int index = info0->sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info0->index == 2){
    for(int i = 0; i <= 2; i++){
      for(int j = 6; j <= 8; j++){
	int index = info0->sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info0->index == 3){
    for(int i = 3; i <= 5; i++){
      for(int j = 0; j <= 2; j++){
	int index = info0->sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info0->index == 4){
    for(int i = 3; i <= 5; i++){
      for(int j = 3; j <= 5; j++){
	int index = info0->sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info0->index == 5){
    for(int i = 3; i <= 5; i++){
      for(int j = 6; j <= 8; j++){
	int index = info0->sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info0->index == 6){
    for(int i = 6; i <= 8; i++){
      for(int j = 0; j <= 2; j++){
	int index = info0->sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info0->index == 7){
      for(int i = 6; i <= 8; i++){
	for(int j = 3; j <= 5; j++){
	int index = info0->sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info0->index == 8){
    for(int i = 6; i <= 8; i++){
      for(int j = 6; j <= 8; j++){
	int index = info0->sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
    
  if((cF[0] == false)| (cF[1] == false)|
     (cF[2] == false)| (cF[3] == false)| (cF[4] == false)|
     (cF[5] == false)| (cF[6] == false)| (cF[7] == false)|
     (cF[8] == false)){
    info0->shared_memory->isValid = false;
    for(int j = 0; j < 9; j++){
      if(cF[j] == false){
	printf("Box %d doesn't have the required values. \n", info0->index+1);	
	break;
      }
    }
  }
}

//**********************************************************
// Function: void CheckBoxFork(void info)                 //
// Description: Checks the specific box as a child        //
//              process and update isValid as needed.     //
//              This code probably should be shortened.   //
//**********************************************************
void CheckBoxFork(ThreadInfo info){
  bool cF[9];
  if(info.index == 0){
    for(int i = 0; i <= 2; i++){
      for(int j = 0; j <= 2; j++){
	int index = info.sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info.index == 1){
    for(int i = 0; i <= 2; i++){
      for(int j = 3; j <= 5; j++){
	int index = info.sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }

  if(info.index == 1){
    for(int i = 0; i <= 2; i++){
      for(int j = 3; j <= 5; j++){
	int index = info.sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info.index == 2){
    for(int i = 0; i <= 2; i++){
      for(int j = 6; j <= 8; j++){
	int index = info.sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info.index == 3){
    for(int i = 3; i <= 5; i++){
      for(int j = 0; j <= 2; j++){
	int index = info.sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info.index == 4){
    for(int i = 3; i <= 5; i++){
      for(int j = 3; j <= 5; j++){
	int index = info.sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info.index == 5){
    for(int i = 3; i <= 5; i++){
      for(int j = 6; j <= 8; j++){
	int index = info.sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info.index == 6){
    for(int i = 6; i <= 8; i++){
      for(int j = 0; j <= 2; j++){
	int index = info.sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info.index == 7){
      for(int i = 6; i <= 8; i++){
	for(int j = 3; j <= 5; j++){
	int index = info.sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
  if(info.index == 8){
    for(int i = 6; i <= 8; i++){
      for(int j = 6; j <= 8; j++){
	int index = info.sudoku_array[i][j]-1;
	cF[index] = true;
      }
    }
  }
    
  if((cF[0] == false)| (cF[1] == false)|
     (cF[2] == false)| (cF[3] == false)| (cF[4] == false)|
     (cF[5] == false)| (cF[6] == false)| (cF[7] == false)|
     (cF[8] == false)){
    info.shared_memory->isValid = false;
    for(int j = 0; j < 9; j++){
      if(cF[j] == false){
	printf("Box %d doesn't have the required values. \n", info.index+1);	
      }
    }
  }
  exit(0);
}
 
//**********************************************************
// Function: void CheckRowFork(void info)                 //
// Description: Checks the specific row as a child        //
//              process and updates isValid as needed.    //
//**********************************************************
void CheckRowFork(ThreadInfo info){
  bool cF[9];
  for(int i = 0; i < 9; i++){
    int index = (info.sudoku_array[info.index][i])-1;
    cF[index] = true;
    if((i == 8) && (cF[0] == false)| (cF[1] == false)|
       (cF[2] == false)| (cF[3] == false)| (cF[4] == false)|
       (cF[5] == false)| (cF[6] == false)| (cF[7] == false)|
       (cF[8] == false)){
      info.shared_memory->isValid = false;
      for(int j = 0; j < 9; j++){
	if(cF[j] == false){
	  printf("Row %d doesn't have the required values. \n", info.index+1);		
	}
      }
    }
  }
}

//**********************************************************
// Function: void CheckColFork(void info)                 //
// Description: Checks the specific col as a child        //
//              process and update isValid as needed.     //
//**********************************************************
void CheckColFork(ThreadInfo info){
  bool cF[9];
  for(int i = 0; i < 9; i++){
    int index = (info.sudoku_array[i][info.index])-1;
    cF[index] = true;
    if((i == 8) && (cF[0] == false)| (cF[1] == false)|
       (cF[2] == false)| (cF[3] == false)| (cF[4] == false)|
       (cF[5] == false)| (cF[6] == false)| (cF[7] == false)|
       (cF[8] == false)){
      info.shared_memory->isValid = false;
      for(int j = 0; j < 9; j++){
	if(cF[j] == false){
	  printf("Col %d doesn't have the required values. \n", info.index+1);		
	}
      }
    }
  }
}

//************************************************************
// Function: void print2dArray(const int matrix[9][9]))     //
// Description: Prints out array, currently unused.         //
//************************************************************
void print2dArray(const int matrix[9][9]){
  for(int i = 0; i < 9; i++){
    for(int j = 0; j < 9; j++){
      printf("%d ", matrix[i][j]);
    }
    printf("\n");
  }
}
