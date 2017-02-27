/*

 Copyright 2011 Julien Cayzac. All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY EXPRESS
 OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 The views and conclusions contained in the software and documentation are
 those of the authors and should not be interpreted as representing official
 policies, either expressed or implied, of the copyright holders.

*/

#pragma once

#ifdef BUILDING_F9
#define F9_PUBLIC __attribute__ ((visibility ("default")))
#else
#define F9_PUBLIC
#endif

typedef struct {
	int x;
	int y;
} F9_CORNER;

#ifdef __cplusplus
extern "C" {
#endif

	/*********/
	/* C API */
	/*********/

	/** Allocate a new detection context.
	  *
	  * @return New context to be used with f9_detect_corners
	  */
	F9_PUBLIC void* f9_alloc();

	/** Deallocate a detection context.
	  *
	  * @param ctx Detection context to deallocate.
	  */
	F9_PUBLIC void f9_dealloc(void* ctx);

	/** Detect corners.
	  *
	  * @param ctx Detection context.
	  * @param image_data A eight-bits-per-pixel gray image.
	  * @param width Image width in pixels (or in bytes).
	  * @param height Image height in pixels (or in bytes).
	  * @param bytes_per_row Stride of the image.
	  * @param threshold Threshold below which differences in luminosity are ignored.
	  * @param suppress_non_max Whether to ignore corners which are neighbors of stronger ones (if true), or be exhaustive (if false).
	  * @param ret_num_corners [out] Upon return, contains the number of corners found.
	  * @return A const array of ret_num_corners corners, owned by ctx.
	  */
	F9_PUBLIC const F9_CORNER* const f9_detect_corners(
	    void* ctx,
	    const unsigned char* image_data,
	    int width,
	    int height,
	    int bytes_per_row,
	    unsigned char threshold,
	    bool suppress_non_max,
	    int* ret_num_corners
	);

#ifdef __cplusplus
}

//
// C++ API
//

#include <vector>

/** Corner detection engine
  */
class F9_PUBLIC F9 {
public:
	F9();
	~F9();

	/** When compiled without exceptions support, this provides a way
	  * to detect the engine is in a working state.
	  */
	operator bool() const;

	/** Detect corners.
	  *
	  * @param image_data A eight-bits-per-pixel gray image.
	  * @param width Image width in pixels (or in bytes).
	  * @param height Image height in pixels (or in bytes).
	  * @param bytes_per_row Stride of the image.
	  * @param threshold Threshold below which differences in luminosity are ignored.
	  * @param suppress_non_max Whether to ignore corners which are neighbors of stronger ones (if true), or be exhaustive (if false).
	  * @return A vector of corners.
	  */
	const std::vector<F9_CORNER>& detectCorners(
	    const unsigned char* image_data,
	    int width,
	    int height,
	    int bytes_per_row,
	    unsigned char threshold,
	    bool suppress_non_max
	);
private:
	F9(const F9&);
	F9& operator=(const F9&);
	class Impl;
	Impl* impl;
};

#endif
