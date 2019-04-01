## C++ Shell

This is a siple command shell supporting most UNIX commands

Piping and multiple pipes are supported, along with a few build in commands

* ptime: total amount of time spent executing child processes
* history: prints a history of all commands
* ^ \<number>: executes the command from history


## Running the program

You must have make and CMake installed in order to run, then run the following commands in the src directory

```bash
mkdir build
cd build
cmake ..
make
./shell
```
