# SudokuChecker
The purpose of this sudoku solution checker was to explore methods of parallel processing using worker threads and child processes, while managing
multiple individual threads or processes without making global varaibles.
# Running
As this was built in C on a Linux machine, we can run it with the following command(s):

make sudoku.x 

./sudoku.x "input_name.txt" (for worker threads)

./sudoku.x "input_name.txt" -f (for child processes)
