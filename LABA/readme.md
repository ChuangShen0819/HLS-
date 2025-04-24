## LabA

### environment
* Xilinx U50
* fireware version : xilinx_u50_gen3x16_xdma_5_202210_1

### script
make run TARGET=hw DEVICE=xilinx_u50_gen3x16_xdma_5_202210_1 LAB=pipeline
make run TARGET=hw DEVICE=xilinx_u50_gen3x16_xdma_5_202210_1 LAB=sync
make run TARGET=hw DEVICE=xilinx_u50_gen3x16_xdma_5_202210_1 LAB=buf

### reference
---
https://github.com/Xilinx/Vitis-Tutorials/tree/2022.1/Hardware_Acceleration/Design_Tutorials/07-host-code-opt