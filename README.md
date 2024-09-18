# Coherent Sharing of Memory-Mapped Files

This project implements a solution for coherent sharing of memory-mapped files across multiple processes, addressing the challenge of concurrent modifications without compromising the integrity of the original file.

## Features

- Copy-on-write mechanism for isolated process writes
- Signal handling for write attempts on read-only mapped regions
- Logging of modifications for each process
- Merging capability to consolidate changes from multiple processes

## Getting Started

### Prerequisites

- GCC compiler
- PTEditor module (https://github.com/misc0110/PTEditor)

### Installation

1. Clone the repository
2. Run `make` to compile the project
3. Load the PTEditor kernel module: `sudo modprobe pteditor`

### Usage

- Initialize the environment: `./psar init`
- Run the test: `./psar test`
- Merge changes:
  - Single log: `./psar merge -s [source_file] -l [log_file]`
  - All logs: `./psar merge_all -s [source_file]`

## Project Structure

```
.
├── app/
├── docs/
├── files/
├── include/
├── logs/
├── merge/
├── src/
├── Makefile
└── README.md
```

## Contributors

- Felipe Paris Mollo Christondis
- Victor Spehar

## Acknowledgments

- Pierre Sens (Project Mentor)
- PTEditor API creators

For more detailed information, please refer to the full report in the `docs/` directory.
