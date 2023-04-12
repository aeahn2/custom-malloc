# Custom Malloc 

A custom wrapper for memory allocation functions `malloc()`, `calloc()`, `realloc()`, and `free()`
***
### Configuration 
Please note that the wrapper does not work natively on MacOS.<br>
Due to limitations of MacOS, the custom malloc library cannot be loaded and called instead of the system's default malloc library.<br>
As a workaround, Mac users can run the project inside a Docker container with the included Dockerfile. 