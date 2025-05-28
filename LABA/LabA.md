
---
### overview
* This lab explores how the host program can influence the overall performance of the application.

* We divide this lab into four parts. The first part explains the kernel function.

* The second to fourth parts demonstrate how the host program can significantly optimize system throughput.

* These include: in-order vs. out-of-order execution, different synchronization strategies, and finally, identifying the optimal (sweet spot) region for memory movement.


### Part 0
* Kernel Function	
``` C
// use arbitrary bit width and dataflow
#include <ap_int.h>
#include <hls_stream.h>
#include <assert.h>
```

* Separate data movement (I/O Processor) from Computation (Processing Element)

<img src="figures/螢幕擷取畫面 2025-04-13 153519.png" width="800">

* Since input and output are physical io ports, it is represented by pointer. And others are used pass by reference.

``` C
// partition kernel into three parts read, write, PE
// read
void read(const ap_int<512>   *input,
    hls::stream<ap_int<512> > &inStream,
	      unsigned int         numInputs) {
  for(unsigned int i = 0; i < numInputs; i++) {
    #pragma HLS PIPELINE
    inStream.write(input[i]);
  }
}

// write
void write(hls::stream<ap_int<512> > &outStream,
	                   ap_int<512>   *output,
	             unsigned int         numInputs) {
  for(unsigned int i = 0; i < numInputs; i++) {
    #pragma HLS PIPELINE
    output[i] = outStream.read();
  }
}

// PE
void exec(hls::stream<ap_int<512> > &inStream,
	      hls::stream<ap_int<512> > &outStream,
	            unsigned int         numInputs,
	            unsigned int         processDelay) {
  for(int num = 0; num < numInputs; num++) {
    ap_int<512> in = inStream.read();
    for(int i = 0; i < processDelay; i++) {
      in += 1;
    }
    outStream.write(in);
  }
}
```

``` c
void pass_dataflow(const ap_int<512> *input,
		   ap_int<512>       *output,
		   unsigned int      numInputs,
		   unsigned int      processDelay) {
  #pragma HLS DATAFLOW

  assert(numInputs >= 1);
  assert(processDelay >=1);

  hls::stream<ap_int<512> > inStream;
  hls::stream<ap_int<512> > outStream;

  read(input,      inStream,  numInputs);
  exec(inStream,   outStream, numInputs, processDelay);
  write(outStream, output,    numInputs);
  
}
```
* The top function must use extern "C" to prevent C++ name mangling, so that the hardware kernel can be correctly linked and called from the host code.

``` C
extern "C" {
void pass(const ap_int<512> *input,
	            ap_int<512> *output,
	      unsigned int       numInputs,
	      unsigned int       processDelay) {
  #pragma HLS INTERFACE m_axi     port=input     offset=slave bundle=p0
  #pragma HLS INTERFACE m_axi     port=output    offset=slave bundle=p1
  
  pass_dataflow(input, output, numInputs, processDelay);
}
}
```
### Part I (Pipeline)

* ***Design.cfg**
``` ini
[connectivity]
nk=pass:1
sp=pass_1.m_axi_p0:DDR[0] # DDR need to be replaced by HBM
sp=pass_1.m_axi_p1:DDR[1] # DDR need to be replaced by HBM
```

* Although using HBM causes the simulation results to differ, the lab still effectively teaches the intended concepts.

* In-order Execution

<img src="figures/螢幕擷取畫面 2025-04-13 195347.png" width="800">

* Out-of-order Execution

``` c
if(oooQueue) {
  std::cout << "Create Out of Order Queue" << std::endl;
  m_queue = clCreateCommandQueue(m_context,
          m_device_id, 
          CL_QUEUE_PROFILING_ENABLE |
          CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE,
          &err);
} else {
  std::cout << "Create Sequential Queue" << std::endl;
  m_queue = clCreateCommandQueue(m_context,
          m_device_id, 
          CL_QUEUE_PROFILING_ENABLE,
          &err);
}    
```

<img src="figures/螢幕擷取畫面 2025-04-13 202650.png" width="800">

<img src="figures/螢幕擷取畫面 2025-04-13 202709.png" width="800">

* Out of order improves throughput significantly, especially without data dependency.


### Part II (Synchronization)

* Synchronize every three execution

``` c
int count = 0;
for(unsigned int i=0; i < numBuffers; i++) {
  count++;
  tasks[i].run(api);
  if(count == 3) {
    count = 0;
    clFinish(api.getQueue());
  }
}
clFinish(api.getQueue());
```

<img src="figures/螢幕擷取畫面 2025-04-13 222648.png" width="600">

<img src="figures/螢幕擷取畫面 2025-04-13 222642.png" width="600">


* Synchronize current execution with the one three positions earlier

``` c
for(unsigned int i=0; i < numBuffers; i++) {
    if(i < 3) {
      tasks[i].run(api);
    } else {
      tasks[i].run(api, tasks[i-3].getDoneEv());
    }
  }
  clFinish(api.getQueue());
```

<img src="figures/螢幕擷取畫面 2025-04-13 222426.png" width="650">

<img src="figures/螢幕擷取畫面 2025-04-13 222433.png" width="650">

* Different synchronization strategies affect throughput. Synchronizing two executions with a distance of three indices (Case 2) performs better than synchronizing every three executions (Case 1), because Case 2 allows synchronization to occur while another kernel is executing(every 3 executions need one finish), whereas Case 1 does not.


### Part III (Buffer)

* In this section, we reuse "Synchronize current execution with the one three positions earlier" and execute 100 times transfer for buffer to get average execution time. We set different buffer size is shown below

* The buffer size can't be larger than $2^{17}$ ($2^{17} * 512/8$ bits)  in this lab. 

<img src="figures/螢幕擷取畫面 2025-04-13 230334.png" width="800">

* A comparison of different buffer sizes ($512 * 2^6$ bits to $512 * 2^{16}$ bits) (unit in figure is byte)

<img src="figures/螢幕擷取畫面 2025-04-13 231048.png" width="600">

* The size is (2 * 512 / 8 bytes) for each execution, and it has 100 executions.

<img src="figures/螢幕擷取畫面 2025-04-17 181014.png" width="800">

<img src="figures/螢幕擷取畫面 2025-04-17 181021.png" width="800">

* The size is ($2^{14}$ * 512 / 8 bytes) for each execution, and it has 100 executions.

<img src="figures/螢幕擷取畫面 2025-04-17 183410.png" width="800">

<img src="figures/螢幕擷取畫面 2025-04-17 183418.png" width="800">

* Since a larger buffer means more data needs to be transferred, there is a 'linear' region where burst transfers are efficient. However, if the buffer is too small or too large, it may hit performance bottlenecks.