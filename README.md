# CS 4348 CPU/Memory Process Project Summary

## Project Purpose
This project simulates a barebones operating system consisting of a CPU that can read from and write to a memory array, and that's it. It's very fundamental, but not simple. The CPU and memory are separate processes that must communicate using pipes because the CPU doesn't have direct access to the memory. The CPU sends requests to the memory through a pipe and the memory process responds by writing to the memory array or sending the CPU a value from memory. 