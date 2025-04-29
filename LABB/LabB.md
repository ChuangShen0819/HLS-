
---
* This lab is to speed up the throughput of 2-D convolution filter.
* To analyze this lab, we can focus on three important speeds, throughput, input memory bandwidth and output memory bandwidth.


## Section I (Vitis_hls)
### Errors you may encounter when running the testbench

* Undefined reference

``` 
/home/ubuntu/Desktop/HLS_project/LabB/Convolution_Filter_v1/solution1/csim/build/../../../hls_testbench.cpp:154: undefined reference to `Filter2D(char const (*) [15], float, short, unsigned short, unsigned short, unsigned short, unsigned char const*, unsigned char*)'
collect2: error: ld returned 1 exit status
```

<img src = "figures/螢幕擷取畫面 2025-04-29 202904.png" width = "800">

<img src = "figures/螢幕擷取畫面 2025-04-29 202919.png" width = "650">

* **Solution**
    Copy Filter2D from filter2d_sw.cpp to hls_testbench.cpp, and rename it as Filter2D_SW.

<img src = "figures/螢幕擷取畫面 2025-04-29 202931.png" width = "650">

<img src = "figures/螢幕擷取畫面 2025-04-29 203005.png" width = "650">

<img src = "figures/螢幕擷取畫面 2025-04-29 205429.png" width = "650">

### C simulation results

<img src = "figures/螢幕擷取畫面 2025-04-29 204158.png" width = "550">

### Synthesis Observations

