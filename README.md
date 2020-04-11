# SudokuChecker
The purpose of this sudoku solution checker was to explore methods of parallel processing using worker threads and child processes, while not using any global variables or directly communicating between threads or processes.
# Running
As this was built in C on a Linux machine, we can run it with the following command(s):

make sudoku.x 

./sudoku.x "input_name.txt" (for worker threads)

./sudoku.x "input_name.txt" -f (for child processes)


You can also skip the compilation step and run sudoku.x (included) with any input file.
