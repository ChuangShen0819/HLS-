/*
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: X11
*/

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <chrono>

#include "cmdlineparser.h"
#include "coefficients.h"
#include "common.h" 

using namespace std;
using namespace sda;
using namespace sda::utils;

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"


void Filter2D_SW(
		const char           coeffs[FILTER_V_SIZE][FILTER_H_SIZE],
		float		         factor,
		short                bias,
		unsigned short       width,
		unsigned short       height,
		unsigned short       stride,
		const unsigned char *src,
		unsigned char       *dst)
{
    for(int y=0; y<height; ++y)
    {
        for(int x=0; x<width; ++x)
        {
        	// Apply 2D filter to the pixel window
			int sum = 0;
			for(int row=0; row<FILTER_V_SIZE; row++)
			{
				for(int col=0; col<FILTER_H_SIZE; col++)
				{
					unsigned char pixel;
					int xoffset = (x+col-(FILTER_H_SIZE/2));
					int yoffset = (y+row-(FILTER_V_SIZE/2));
					// Deal with boundary conditions : clamp pixels to 0 when outside of image
					if ( (xoffset<0) || (xoffset>=width) || (yoffset<0) || (yoffset>=height) ) {
						pixel = 0;
					} else {
						pixel = src[yoffset*stride+xoffset];
					}
					sum += pixel*coeffs[row][col];
				}
			}

        	// Normalize and saturate result
			unsigned char outpix = MIN(MAX((int(factor * sum)+bias), 0), 255);

			// Write output
           	dst[y*stride+x] = outpix;
        }
    }
}


int main(int argc, char** argv)
{
  printf("----------------------------------------------------------------------------\n");
  printf("HLS Testbench for Xilinx 2D Filter Example\n");
        
  // ---------------------------------------------------------------------------------
  // Parse command line
  // ---------------------------------------------------------------------------------
  CmdLineParser parser;
  parser.addSwitch("--width",  "-w", "Image width", "1000");
  parser.addSwitch("--height", "-h", "Image height",  "30");

  //parse all command line options
  parser.parse(argc, argv);
  unsigned width   = parser.value_to_int("width");
  unsigned height  = parser.value_to_int("height");
  unsigned stride  = ceil(width/64.0)*64;
  unsigned nbytes  = stride*height; //(stride*height);

  printf("Image info\n");
  printf("- Width     : %10d\n", width);
  printf("- Height    : %10d\n", height);
  printf("- Stride    : %10d\n", stride);
  printf("- Bytes     : %10d\n", nbytes);

  // Input and output buffers (Y,U, V)
  unsigned char *y_src = (unsigned char *)malloc(MAX_IMAGE_WIDTH*MAX_IMAGE_HEIGHT);
  unsigned char *u_src = (unsigned char *)malloc(MAX_IMAGE_WIDTH*MAX_IMAGE_HEIGHT);
  unsigned char *v_src = (unsigned char *)malloc(MAX_IMAGE_WIDTH*MAX_IMAGE_HEIGHT);
  unsigned char *y_dst = (unsigned char *)malloc(MAX_IMAGE_WIDTH*MAX_IMAGE_HEIGHT);
  unsigned char *u_dst = (unsigned char *)malloc(MAX_IMAGE_WIDTH*MAX_IMAGE_HEIGHT);
  unsigned char *v_dst = (unsigned char *)malloc(MAX_IMAGE_WIDTH*MAX_IMAGE_HEIGHT);

  // Coefficients
  char coeffs[FILTER_V_SIZE][FILTER_H_SIZE];

  for (int i=0; i<nbytes; i++) {
  	y_src[i] = rand();
  	u_src[i] = rand();
  	v_src[i] = rand();
  	y_dst[i] = 0;
  	u_dst[i] = 0;
  	v_dst[i] = 0;
  }

  for(int row=0; row<FILTER_V_SIZE; row++) {
    for(int col=0; col<FILTER_H_SIZE; col++) {
      coeffs[row][col]=1;
    }
  }

  unsigned num_coefs = FILTER_V_SIZE*FILTER_H_SIZE;
  unsigned num_coefs_padded = (((num_coefs-1)/64)+1)*64;
  char *coeffs_padded = (char *)malloc(num_coefs_padded);
  memcpy(coeffs_padded, &coeffs[0][0], num_coefs);

  // filter factor and bias
  float factor = 1.0/(FILTER_V_SIZE*FILTER_H_SIZE);
  short bias   = 0;  

  // ---------------------------------------------------------------------------------
  // Run the FPGA kernels
  // ---------------------------------------------------------------------------------

  printf("Running FPGA accelerator\n");    
 
  Filter2DKernel(coeffs_padded, factor, bias, width, height, stride, y_src, y_dst);
  // Filter2DKernel(coeffs_padded, factor, bias, width, height, stride, u_src, u_dst);
  // Filter2DKernel(coeffs_padded, factor, bias, width, height, stride, v_src, v_dst);


  // ---------------------------------------------------------------------------------
  // Compute reference results and compare
  // ---------------------------------------------------------------------------------

  printf("Comparing results\n");    

  // Create output buffers for reference results
  unsigned char *y_ref = (unsigned char *)malloc(nbytes);
  unsigned char *u_ref = (unsigned char *)malloc(nbytes);
  unsigned char *v_ref = (unsigned char *)malloc(nbytes);

  // Compute reference results
  Filter2D_SW(coeffs, factor, bias, width, height, stride, y_src, y_ref);
  // Filter2D_SW(coeffs, factor, bias, width, height, stride, u_src, u_ref);
  // Filter2D_SW(coeffs, factor, bias, width, height, stride, v_src, v_ref);

  // Compare results
  bool diff = false;
  for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
          if ( y_dst[y*stride+x] != y_ref[y*stride+x] ) diff = true;
          // if ( u_dst[y*stride+x] != u_ref[y*stride+x] ) diff = true;
          // if ( v_dst[y*stride+x] != v_ref[y*stride+x] ) diff = true;
      }
  }

  if(diff) {
      printf("%sTest FAILED: Output has mismatches with reference%s\n", RED, RESET);    
  } else {
      printf("%sTest PASSED: Output matches reference%s\n", GREEN, RESET);    
  }
  printf("----------------------------------------------------------------------------\n"); 

  return (diff?1:0);
}


