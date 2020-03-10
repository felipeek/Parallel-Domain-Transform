# Parallel-Domain-Transform
The domain transform filter implemented using parallelism.

## Building

Makefile is available for OSX and Linux. To build, run:

```bash
$ make
```

The result will be available in `bin/parallel-domain-transform`.

## Usage

```bash
$ ./parallel-domain-transform -h
Usage: ./parallel-domain-transform

Optional Parameters:
	Image Path: -p <string>
	Spatial Factor: -s <r32>
	Range Factor: -r <r32>
	Number of Iterations: -i <s32>
	Parallelism Level (X Axis): -x <s32>
	Parallelism Level (Y Axis): -y <s32>
	Number of Threads (MAX 64): -t <s32>
	Collect Time: -c <1 or 0>
	Result Path: -o <string>
	Help: -h
  ```

 ## Example

 ```bash
 $ ./parallel-domain-transform -p image.png -s 10 -r 0.5 -i 3 -x 4 -y 4 -t 16 -o ../res/output.png
 ```
