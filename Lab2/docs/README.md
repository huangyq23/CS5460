# Master-Worker API

## Getting Started

## API Reference

### Functions
#### [MW_Run](id:MW_Run)
```
void MW_Run (int argc, char **argv, struct mw_api_spec *spec);
```
This is the main entry point of the API. The command line arguments `argc` and `argv` should be passed in by the caller. The `f` parameter is the work specification given by user.
##### Parameters
 Name | Type | Value 
 -----|------|------- 
 argc | int | Command-line arguments 
 argv | char ** | Command-line arguments 
 spec | [mw_compute_func](#mw_api_spec) *| The user-defined work specification.

### Types

#### [mw_api_spec](id:mw_api_spec)
```
struct mw_api_spec
{
    mw_create_func create;
    mw_result_func result;
    mw_compute_func compute;
    int work_sz;
    int res_sz;
    int meta_sz;
};
```
##### Members
 Name | Type | Value 
 -----|------|-------
 create |[mw_create_func](#mw_create_func) | User-defined work creation function
 result | [mw_result_func](#mw_result_func)| User-defined result processing function
 compute | [mw_compute_func](#mw_compute_func)| User-defined work computation function
 work_sz | int | Work size in bytes
 res_sz | int | Result size in bytes
 meta_sz | int | Metadata size in bytes

#### [mw_works](id:mw_works)
```
struct mw_works
{
	int size;
	void *works;
};
```
##### Members
 Name | Type | Value 
 -----|------|-------
 size | int | Number of works in work array
 works | void * | Work array

### User-defined Functions

There are three user functions required in the specification.

* `mw_create_func` and `mw_result_func` are excuted in the master process.

* `mw_compute_func` is excuted in worker processes.

#### [mw_create_func](id:mw_create_func) *create*
```
mw_works* create (int argc, char **argv, void *meta);
```
The `mw_create_func` is where the user need to create a list of work to be processed.
The `argc` and `argv` are command line options. (Or, whatever passed to [`MW_Run`](#MW_Run)).
The `meta` is the user defined meta data struct that will persist across [`mw_create_func`](#mw_create_func) and [`mw_compute_func`](#mw_compute_func) calls. 
##### Parameters
 Name | Type | Value 
 -----|------|------- 
 argc | int | Command-line arguments 
 argv | char ** | Command-line arguments 
 meta | void * | User-defined metadat

##### Return
 Type | Value 
 -----|------ 
[mw_works](#mw_works) * | The generated work

#### [mw_result_func](id:mw_result_func) *result*
```
int result (int sz, void *res, void *meta);
```
##### Parameters
 Name | Type | Value 
 -----|------|------- 
 sz | int | Number of results in result array
 res | void * | Result array
 meta | void * | User-defined metadata
##### Return
 Type| Value 
 ----|------ 
 int | Status code

#### [mw_compute_func](id:mw_compute_func) *compute*
```
void* compute(void *work);
```
##### Parameters
 Name | Type | Value 
 -----|------|------- 
 work | void * | A single work to be computed
##### Return
 Type | Value 
 ----|------- 
 void * | Computed result, `NULL` if there is no result.
