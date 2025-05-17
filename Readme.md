Virtual Machines
================

Implementations for (roughly) the virtual machines described in 
> Seidl, Helmut and  Wilhelm, Reinhard [*Compiler Design — Virtual Machines*](https://link.springer.com/book/10.1007/978-3-642-14909-2), Springer 2010.

## The C-Machine

Build (on Linux, requires CMake and Make):

    mkdir build && cd build && cmake .. && make 

And to execute: 

    ./cma program.cvm

Or just use:

    ./run.sh program.cvm 
