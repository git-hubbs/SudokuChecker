# SudokuChecker
The purpose of this sudoku solution checker was to explore methods of parallel processing using worker threads and child processes for my OS class, while not directly communicating between them or using any global variables. 

# Running
As this was built in C in a Linux environment, we can run it with the following command(s):

make sudoku.x 

./sudoku.x "input_name.txt" (for worker threads)

./sudoku.x "input_name.txt" -f (for child processes)


You can also skip the make step and simply run the included precompiled sudoku.x with an input file.
