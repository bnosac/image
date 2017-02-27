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

#define BUILDING_F9
#include "f9.h"

class F9::Impl {
private:
	std::vector<F9_CORNER>  ret_corners;
	std::vector<F9_CORNER>  nonmax;
	std::vector<int>       scores;
	std::vector<int>       row_start;

	static inline void makeOffsets(int pixel[], int stride) {
		pixel[ 0] =  0 + stride *  3;
		pixel[ 1] =  1 + stride *  3;
		pixel[ 2] =  2 + stride *  2;
		pixel[ 3] =  3 + stride *  1;
		pixel[ 4] =  3 + stride *  0;
		pixel[ 5] =  3 + stride * -1;
		pixel[ 6] =  2 + stride * -2;
		pixel[ 7] =  1 + stride * -3;
		pixel[ 8] =  0 + stride * -3;
		pixel[ 9] = -1 + stride * -3;
		pixel[10] = -2 + stride * -2;
		pixel[11] = -3 + stride * -1;
		pixel[12] = -3 + stride *  0;
		pixel[13] = -3 + stride *  1;
		pixel[14] = -2 + stride *  2;
		pixel[15] = -1 + stride *  3;
	}

	Impl(const Impl&);
	Impl& operator=(const Impl&);
public:
	Impl() { }

	const std::vector<F9_CORNER>& detectCorners(
	    const unsigned char* image_data,
	    int width,
	    int height,
	    int bytes_per_row,
	    unsigned char threshold,
	    bool suppress_non_max
	) {
		detectAllCorners(image_data, width, height, bytes_per_row, threshold);

		if(!suppress_non_max)
			return ret_corners;

		cornersScores(image_data, bytes_per_row, threshold);
		nonMaxSuppression();
		return nonmax;
	}

	void nonMaxSuppression() {
		nonmax.clear();

		if(ret_corners.empty())
			return;

		// Find where each row begins
		// (the corners are output in raster scan order). A beginning of -1 signifies
		// that there are no corners on that row.
		int last_row = ret_corners.back().y;
		row_start.resize(last_row + 1);

		for(int i(0); i < last_row + 1; ++i)
			row_start[i] = -1;

		int prev_row = -1;

		for(int i(0); i < (int) ret_corners.size(); ++i)
			if(ret_corners[i].y != prev_row) {
				row_start[ret_corners[i].y] = i;
				prev_row = ret_corners[i].y;
			}

		// Point above points (roughly) to the pixel above the one of interest,
		// if there is a feature there.
		int point_above(0), point_below(0);

		for(int i(0); i < (int) ret_corners.size(); i++) {
			int score = scores[i];
			const F9_CORNER& pos = ret_corners[i];

			// Check left
			if(i > 0)
				if(ret_corners[i - 1].x == pos.x - 1 && ret_corners[i - 1].y == pos.y && (scores[i - 1] >= score))
					continue;

			// Check right
			if(i + 1 < (int) ret_corners.size())
				if(ret_corners[i + 1].x == pos.x + 1 && ret_corners[i + 1].y == pos.y && (scores[i + 1] >= score))
					continue;

			/*Check above (if there is a valid row above)*/
			if(pos.y != 0 && row_start[pos.y - 1] != -1) {
				/*Make sure that current point_above is one
				  row above.*/
				if(ret_corners[point_above].y < pos.y - 1)
					point_above = row_start[pos.y - 1];

				/*Make point_above point to the first of the pixels above the current point,
				  if it exists.*/
				for(; ret_corners[point_above].y < pos.y && ret_corners[point_above].x < pos.x - 1; point_above++)
					{}

				bool cont(false);

				for(int j(point_above); ret_corners[j].y < pos.y && ret_corners[j].x <= pos.x + 1 && !cont; j++) {
					int x = ret_corners[j].x;
					cont = (x == pos.x - 1 || x == pos.x || x == pos.x + 1) && (scores[j] >= score);
				}

				if(cont) continue;
			}

			/*Check below (if there is anything below)*/
			if(pos.y != last_row && row_start[pos.y + 1] != -1 && point_below < (int) ret_corners.size()) { /*Nothing below*/
				if(ret_corners[point_below].y < pos.y + 1)
					point_below = row_start[pos.y + 1];

				/* Make point below point to one of the pixels belowthe current point, if it
				   exists.*/
				for(; point_below < (int) ret_corners.size() && ret_corners[point_below].y == pos.y + 1 && ret_corners[point_below].x < pos.x - 1; point_below++)
					{}

				bool cont(false);

				for(int j(point_below); j < (int) ret_corners.size() && ret_corners[j].y == pos.y + 1 && ret_corners[j].x <= pos.x + 1 && !cont; j++) {
					int x = ret_corners[j].x;
					cont = (x == pos.x - 1 || x == pos.x || x == pos.x + 1) && (scores[j] >= score);
				}

				if(cont) continue;
			}

			nonmax.push_back(ret_corners[i]);
		}
	}

	int cornerScore(const unsigned char* p, const int pixel[], unsigned char bstart) {
		unsigned char bmin(bstart);
		unsigned char bmax(255);
		unsigned char b(((int)bmax + (int)bmin) >> 1);

		// Compute the score using binary search
		for(;;) {
			const unsigned char cb((*p > 255 - b) ? 255 : *p + b);
			const unsigned char c_b((*p < b) ? 0 : *p - b);

			if(p[pixel[0]] > cb)
				if(p[pixel[1]] > cb)
					if(p[pixel[2]] > cb)
						if(p[pixel[3]] > cb)
							if(p[pixel[4]] > cb)
								if(p[pixel[5]] > cb)
									if(p[pixel[6]] > cb)
										if(p[pixel[7]] > cb)
											if(p[pixel[8]] > cb)
												goto is_a_corner;
											else if(p[pixel[15]] > cb)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[7]] < c_b)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[14]] < c_b)
												if(p[pixel[8]] < c_b)
													if(p[pixel[9]] < c_b)
														if(p[pixel[10]] < c_b)
															if(p[pixel[11]] < c_b)
																if(p[pixel[12]] < c_b)
																	if(p[pixel[13]] < c_b)
																		if(p[pixel[15]] < c_b)
																			goto is_a_corner;
																		else
																			goto is_not_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[14]] > cb)
											if(p[pixel[15]] > cb)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[6]] < c_b)
										if(p[pixel[15]] > cb)
											if(p[pixel[13]] > cb)
												if(p[pixel[14]] > cb)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[13]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														if(p[pixel[9]] < c_b)
															if(p[pixel[10]] < c_b)
																if(p[pixel[11]] < c_b)
																	if(p[pixel[12]] < c_b)
																		if(p[pixel[14]] < c_b)
																			goto is_a_corner;
																		else
																			goto is_not_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[7]] < c_b)
											if(p[pixel[8]] < c_b)
												if(p[pixel[9]] < c_b)
													if(p[pixel[10]] < c_b)
														if(p[pixel[11]] < c_b)
															if(p[pixel[12]] < c_b)
																if(p[pixel[13]] < c_b)
																	if(p[pixel[14]] < c_b)
																		goto is_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[13]] > cb)
										if(p[pixel[14]] > cb)
											if(p[pixel[15]] > cb)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[13]] < c_b)
										if(p[pixel[7]] < c_b)
											if(p[pixel[8]] < c_b)
												if(p[pixel[9]] < c_b)
													if(p[pixel[10]] < c_b)
														if(p[pixel[11]] < c_b)
															if(p[pixel[12]] < c_b)
																if(p[pixel[14]] < c_b)
																	if(p[pixel[15]] < c_b)
																		goto is_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[5]] < c_b)
									if(p[pixel[14]] > cb)
										if(p[pixel[12]] > cb)
											if(p[pixel[13]] > cb)
												if(p[pixel[15]] > cb)
													goto is_a_corner;
												else if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb)
																if(p[pixel[10]] > cb)
																	if(p[pixel[11]] > cb)
																		goto is_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[12]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														if(p[pixel[9]] < c_b)
															if(p[pixel[10]] < c_b)
																if(p[pixel[11]] < c_b)
																	if(p[pixel[13]] < c_b)
																		goto is_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[14]] < c_b)
										if(p[pixel[7]] < c_b)
											if(p[pixel[8]] < c_b)
												if(p[pixel[9]] < c_b)
													if(p[pixel[10]] < c_b)
														if(p[pixel[11]] < c_b)
															if(p[pixel[12]] < c_b)
																if(p[pixel[13]] < c_b)
																	if(p[pixel[6]] < c_b)
																		goto is_a_corner;
																	else if(p[pixel[15]] < c_b)
																		goto is_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[6]] < c_b)
										if(p[pixel[7]] < c_b)
											if(p[pixel[8]] < c_b)
												if(p[pixel[9]] < c_b)
													if(p[pixel[10]] < c_b)
														if(p[pixel[11]] < c_b)
															if(p[pixel[12]] < c_b)
																if(p[pixel[13]] < c_b)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[12]] > cb)
									if(p[pixel[13]] > cb)
										if(p[pixel[14]] > cb)
											if(p[pixel[15]] > cb)
												goto is_a_corner;
											else if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														if(p[pixel[9]] > cb)
															if(p[pixel[10]] > cb)
																if(p[pixel[11]] > cb)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[12]] < c_b)
									if(p[pixel[7]] < c_b)
										if(p[pixel[8]] < c_b)
											if(p[pixel[9]] < c_b)
												if(p[pixel[10]] < c_b)
													if(p[pixel[11]] < c_b)
														if(p[pixel[13]] < c_b)
															if(p[pixel[14]] < c_b)
																if(p[pixel[6]] < c_b)
																	goto is_a_corner;
																else if(p[pixel[15]] < c_b)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else if(p[pixel[4]] < c_b)
								if(p[pixel[13]] > cb)
									if(p[pixel[11]] > cb)
										if(p[pixel[12]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb)
													goto is_a_corner;
												else if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb)
																if(p[pixel[10]] > cb)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb)
																if(p[pixel[10]] > cb)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[11]] < c_b)
										if(p[pixel[5]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														if(p[pixel[9]] < c_b)
															if(p[pixel[10]] < c_b)
																if(p[pixel[12]] < c_b)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[13]] < c_b)
									if(p[pixel[7]] < c_b)
										if(p[pixel[8]] < c_b)
											if(p[pixel[9]] < c_b)
												if(p[pixel[10]] < c_b)
													if(p[pixel[11]] < c_b)
														if(p[pixel[12]] < c_b)
															if(p[pixel[6]] < c_b)
																if(p[pixel[5]] < c_b)
																	goto is_a_corner;
																else if(p[pixel[14]] < c_b)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else if(p[pixel[14]] < c_b)
																if(p[pixel[15]] < c_b)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[5]] < c_b)
									if(p[pixel[6]] < c_b)
										if(p[pixel[7]] < c_b)
											if(p[pixel[8]] < c_b)
												if(p[pixel[9]] < c_b)
													if(p[pixel[10]] < c_b)
														if(p[pixel[11]] < c_b)
															if(p[pixel[12]] < c_b)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else if(p[pixel[11]] > cb)
								if(p[pixel[12]] > cb)
									if(p[pixel[13]] > cb)
										if(p[pixel[14]] > cb)
											if(p[pixel[15]] > cb)
												goto is_a_corner;
											else if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														if(p[pixel[9]] > cb)
															if(p[pixel[10]] > cb)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[5]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														if(p[pixel[9]] > cb)
															if(p[pixel[10]] > cb)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else if(p[pixel[11]] < c_b)
								if(p[pixel[7]] < c_b)
									if(p[pixel[8]] < c_b)
										if(p[pixel[9]] < c_b)
											if(p[pixel[10]] < c_b)
												if(p[pixel[12]] < c_b)
													if(p[pixel[13]] < c_b)
														if(p[pixel[6]] < c_b)
															if(p[pixel[5]] < c_b)
																goto is_a_corner;
															else if(p[pixel[14]] < c_b)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else if(p[pixel[14]] < c_b)
															if(p[pixel[15]] < c_b)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else if(p[pixel[3]] < c_b)
							if(p[pixel[10]] > cb)
								if(p[pixel[11]] > cb)
									if(p[pixel[12]] > cb)
										if(p[pixel[13]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb)
													goto is_a_corner;
												else if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[4]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else if(p[pixel[10]] < c_b)
								if(p[pixel[7]] < c_b)
									if(p[pixel[8]] < c_b)
										if(p[pixel[9]] < c_b)
											if(p[pixel[11]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[5]] < c_b)
														if(p[pixel[4]] < c_b)
															goto is_a_corner;
														else if(p[pixel[12]] < c_b)
															if(p[pixel[13]] < c_b)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else if(p[pixel[12]] < c_b)
														if(p[pixel[13]] < c_b)
															if(p[pixel[14]] < c_b)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else if(p[pixel[12]] < c_b)
													if(p[pixel[13]] < c_b)
														if(p[pixel[14]] < c_b)
															if(p[pixel[15]] < c_b)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else if(p[pixel[10]] > cb)
							if(p[pixel[11]] > cb)
								if(p[pixel[12]] > cb)
									if(p[pixel[13]] > cb)
										if(p[pixel[14]] > cb)
											if(p[pixel[15]] > cb)
												goto is_a_corner;
											else if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														if(p[pixel[9]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[5]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														if(p[pixel[9]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[4]] > cb)
										if(p[pixel[5]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														if(p[pixel[9]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else if(p[pixel[10]] < c_b)
							if(p[pixel[7]] < c_b)
								if(p[pixel[8]] < c_b)
									if(p[pixel[9]] < c_b)
										if(p[pixel[11]] < c_b)
											if(p[pixel[12]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[5]] < c_b)
														if(p[pixel[4]] < c_b)
															goto is_a_corner;
														else if(p[pixel[13]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else if(p[pixel[13]] < c_b)
														if(p[pixel[14]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else if(p[pixel[13]] < c_b)
													if(p[pixel[14]] < c_b)
														if(p[pixel[15]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else if(p[pixel[2]] < c_b)
						if(p[pixel[9]] > cb)
							if(p[pixel[10]] > cb)
								if(p[pixel[11]] > cb)
									if(p[pixel[12]] > cb)
										if(p[pixel[13]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb)
													goto is_a_corner;
												else if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[4]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[3]] > cb)
										if(p[pixel[4]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else if(p[pixel[9]] < c_b)
							if(p[pixel[7]] < c_b)
								if(p[pixel[8]] < c_b)
									if(p[pixel[10]] < c_b)
										if(p[pixel[6]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[4]] < c_b)
													if(p[pixel[3]] < c_b)
														goto is_a_corner;
													else if(p[pixel[11]] < c_b)
														if(p[pixel[12]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else if(p[pixel[11]] < c_b)
													if(p[pixel[12]] < c_b)
														if(p[pixel[13]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[11]] < c_b)
												if(p[pixel[12]] < c_b)
													if(p[pixel[13]] < c_b)
														if(p[pixel[14]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[11]] < c_b)
											if(p[pixel[12]] < c_b)
												if(p[pixel[13]] < c_b)
													if(p[pixel[14]] < c_b)
														if(p[pixel[15]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else if(p[pixel[9]] > cb)
						if(p[pixel[10]] > cb)
							if(p[pixel[11]] > cb)
								if(p[pixel[12]] > cb)
									if(p[pixel[13]] > cb)
										if(p[pixel[14]] > cb)
											if(p[pixel[15]] > cb)
												goto is_a_corner;
											else if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[5]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[4]] > cb)
										if(p[pixel[5]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[3]] > cb)
									if(p[pixel[4]] > cb)
										if(p[pixel[5]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else if(p[pixel[9]] < c_b)
						if(p[pixel[7]] < c_b)
							if(p[pixel[8]] < c_b)
								if(p[pixel[10]] < c_b)
									if(p[pixel[11]] < c_b)
										if(p[pixel[6]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[4]] < c_b)
													if(p[pixel[3]] < c_b)
														goto is_a_corner;
													else if(p[pixel[12]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else if(p[pixel[12]] < c_b)
													if(p[pixel[13]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[12]] < c_b)
												if(p[pixel[13]] < c_b)
													if(p[pixel[14]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[12]] < c_b)
											if(p[pixel[13]] < c_b)
												if(p[pixel[14]] < c_b)
													if(p[pixel[15]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else
						goto is_not_a_corner;
				else if(p[pixel[1]] < c_b)
					if(p[pixel[8]] > cb)
						if(p[pixel[9]] > cb)
							if(p[pixel[10]] > cb)
								if(p[pixel[11]] > cb)
									if(p[pixel[12]] > cb)
										if(p[pixel[13]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb)
													goto is_a_corner;
												else if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[4]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[3]] > cb)
										if(p[pixel[4]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[2]] > cb)
									if(p[pixel[3]] > cb)
										if(p[pixel[4]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else if(p[pixel[8]] < c_b)
						if(p[pixel[7]] < c_b)
							if(p[pixel[9]] < c_b)
								if(p[pixel[6]] < c_b)
									if(p[pixel[5]] < c_b)
										if(p[pixel[4]] < c_b)
											if(p[pixel[3]] < c_b)
												if(p[pixel[2]] < c_b)
													goto is_a_corner;
												else if(p[pixel[10]] < c_b)
													if(p[pixel[11]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[10]] < c_b)
												if(p[pixel[11]] < c_b)
													if(p[pixel[12]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[10]] < c_b)
											if(p[pixel[11]] < c_b)
												if(p[pixel[12]] < c_b)
													if(p[pixel[13]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[10]] < c_b)
										if(p[pixel[11]] < c_b)
											if(p[pixel[12]] < c_b)
												if(p[pixel[13]] < c_b)
													if(p[pixel[14]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[10]] < c_b)
									if(p[pixel[11]] < c_b)
										if(p[pixel[12]] < c_b)
											if(p[pixel[13]] < c_b)
												if(p[pixel[14]] < c_b)
													if(p[pixel[15]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else
						goto is_not_a_corner;
				else if(p[pixel[8]] > cb)
					if(p[pixel[9]] > cb)
						if(p[pixel[10]] > cb)
							if(p[pixel[11]] > cb)
								if(p[pixel[12]] > cb)
									if(p[pixel[13]] > cb)
										if(p[pixel[14]] > cb)
											if(p[pixel[15]] > cb)
												goto is_a_corner;
											else if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[5]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[4]] > cb)
										if(p[pixel[5]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[3]] > cb)
									if(p[pixel[4]] > cb)
										if(p[pixel[5]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else if(p[pixel[2]] > cb)
								if(p[pixel[3]] > cb)
									if(p[pixel[4]] > cb)
										if(p[pixel[5]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else
						goto is_not_a_corner;
				else if(p[pixel[8]] < c_b)
					if(p[pixel[7]] < c_b)
						if(p[pixel[9]] < c_b)
							if(p[pixel[10]] < c_b)
								if(p[pixel[6]] < c_b)
									if(p[pixel[5]] < c_b)
										if(p[pixel[4]] < c_b)
											if(p[pixel[3]] < c_b)
												if(p[pixel[2]] < c_b)
													goto is_a_corner;
												else if(p[pixel[11]] < c_b)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[11]] < c_b)
												if(p[pixel[12]] < c_b)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[11]] < c_b)
											if(p[pixel[12]] < c_b)
												if(p[pixel[13]] < c_b)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[11]] < c_b)
										if(p[pixel[12]] < c_b)
											if(p[pixel[13]] < c_b)
												if(p[pixel[14]] < c_b)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[11]] < c_b)
									if(p[pixel[12]] < c_b)
										if(p[pixel[13]] < c_b)
											if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else
						goto is_not_a_corner;
				else
					goto is_not_a_corner;
			else if(p[pixel[0]] < c_b)
				if(p[pixel[1]] > cb)
					if(p[pixel[8]] > cb)
						if(p[pixel[7]] > cb)
							if(p[pixel[9]] > cb)
								if(p[pixel[6]] > cb)
									if(p[pixel[5]] > cb)
										if(p[pixel[4]] > cb)
											if(p[pixel[3]] > cb)
												if(p[pixel[2]] > cb)
													goto is_a_corner;
												else if(p[pixel[10]] > cb)
													if(p[pixel[11]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[10]] > cb)
												if(p[pixel[11]] > cb)
													if(p[pixel[12]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[10]] > cb)
											if(p[pixel[11]] > cb)
												if(p[pixel[12]] > cb)
													if(p[pixel[13]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[10]] > cb)
										if(p[pixel[11]] > cb)
											if(p[pixel[12]] > cb)
												if(p[pixel[13]] > cb)
													if(p[pixel[14]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[10]] > cb)
									if(p[pixel[11]] > cb)
										if(p[pixel[12]] > cb)
											if(p[pixel[13]] > cb)
												if(p[pixel[14]] > cb)
													if(p[pixel[15]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else if(p[pixel[8]] < c_b)
						if(p[pixel[9]] < c_b)
							if(p[pixel[10]] < c_b)
								if(p[pixel[11]] < c_b)
									if(p[pixel[12]] < c_b)
										if(p[pixel[13]] < c_b)
											if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b)
													goto is_a_corner;
												else if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[4]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[3]] < c_b)
										if(p[pixel[4]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[2]] < c_b)
									if(p[pixel[3]] < c_b)
										if(p[pixel[4]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else
						goto is_not_a_corner;
				else if(p[pixel[1]] < c_b)
					if(p[pixel[2]] > cb)
						if(p[pixel[9]] > cb)
							if(p[pixel[7]] > cb)
								if(p[pixel[8]] > cb)
									if(p[pixel[10]] > cb)
										if(p[pixel[6]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[4]] > cb)
													if(p[pixel[3]] > cb)
														goto is_a_corner;
													else if(p[pixel[11]] > cb)
														if(p[pixel[12]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else if(p[pixel[11]] > cb)
													if(p[pixel[12]] > cb)
														if(p[pixel[13]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[11]] > cb)
												if(p[pixel[12]] > cb)
													if(p[pixel[13]] > cb)
														if(p[pixel[14]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[11]] > cb)
											if(p[pixel[12]] > cb)
												if(p[pixel[13]] > cb)
													if(p[pixel[14]] > cb)
														if(p[pixel[15]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else if(p[pixel[9]] < c_b)
							if(p[pixel[10]] < c_b)
								if(p[pixel[11]] < c_b)
									if(p[pixel[12]] < c_b)
										if(p[pixel[13]] < c_b)
											if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b)
													goto is_a_corner;
												else if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[4]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[3]] < c_b)
										if(p[pixel[4]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else if(p[pixel[2]] < c_b)
						if(p[pixel[3]] > cb)
							if(p[pixel[10]] > cb)
								if(p[pixel[7]] > cb)
									if(p[pixel[8]] > cb)
										if(p[pixel[9]] > cb)
											if(p[pixel[11]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[5]] > cb)
														if(p[pixel[4]] > cb)
															goto is_a_corner;
														else if(p[pixel[12]] > cb)
															if(p[pixel[13]] > cb)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else if(p[pixel[12]] > cb)
														if(p[pixel[13]] > cb)
															if(p[pixel[14]] > cb)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else if(p[pixel[12]] > cb)
													if(p[pixel[13]] > cb)
														if(p[pixel[14]] > cb)
															if(p[pixel[15]] > cb)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else if(p[pixel[10]] < c_b)
								if(p[pixel[11]] < c_b)
									if(p[pixel[12]] < c_b)
										if(p[pixel[13]] < c_b)
											if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b)
													goto is_a_corner;
												else if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[4]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else if(p[pixel[3]] < c_b)
							if(p[pixel[4]] > cb)
								if(p[pixel[13]] > cb)
									if(p[pixel[7]] > cb)
										if(p[pixel[8]] > cb)
											if(p[pixel[9]] > cb)
												if(p[pixel[10]] > cb)
													if(p[pixel[11]] > cb)
														if(p[pixel[12]] > cb)
															if(p[pixel[6]] > cb)
																if(p[pixel[5]] > cb)
																	goto is_a_corner;
																else if(p[pixel[14]] > cb)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else if(p[pixel[14]] > cb)
																if(p[pixel[15]] > cb)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[13]] < c_b)
									if(p[pixel[11]] > cb)
										if(p[pixel[5]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														if(p[pixel[9]] > cb)
															if(p[pixel[10]] > cb)
																if(p[pixel[12]] > cb)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[11]] < c_b)
										if(p[pixel[12]] < c_b)
											if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b)
													goto is_a_corner;
												else if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b)
																if(p[pixel[10]] < c_b)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b)
																if(p[pixel[10]] < c_b)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[5]] > cb)
									if(p[pixel[6]] > cb)
										if(p[pixel[7]] > cb)
											if(p[pixel[8]] > cb)
												if(p[pixel[9]] > cb)
													if(p[pixel[10]] > cb)
														if(p[pixel[11]] > cb)
															if(p[pixel[12]] > cb)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else if(p[pixel[4]] < c_b)
								if(p[pixel[5]] > cb)
									if(p[pixel[14]] > cb)
										if(p[pixel[7]] > cb)
											if(p[pixel[8]] > cb)
												if(p[pixel[9]] > cb)
													if(p[pixel[10]] > cb)
														if(p[pixel[11]] > cb)
															if(p[pixel[12]] > cb)
																if(p[pixel[13]] > cb)
																	if(p[pixel[6]] > cb)
																		goto is_a_corner;
																	else if(p[pixel[15]] > cb)
																		goto is_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[14]] < c_b)
										if(p[pixel[12]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														if(p[pixel[9]] > cb)
															if(p[pixel[10]] > cb)
																if(p[pixel[11]] > cb)
																	if(p[pixel[13]] > cb)
																		goto is_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[12]] < c_b)
											if(p[pixel[13]] < c_b)
												if(p[pixel[15]] < c_b)
													goto is_a_corner;
												else if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b)
																if(p[pixel[10]] < c_b)
																	if(p[pixel[11]] < c_b)
																		goto is_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[6]] > cb)
										if(p[pixel[7]] > cb)
											if(p[pixel[8]] > cb)
												if(p[pixel[9]] > cb)
													if(p[pixel[10]] > cb)
														if(p[pixel[11]] > cb)
															if(p[pixel[12]] > cb)
																if(p[pixel[13]] > cb)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[5]] < c_b)
									if(p[pixel[6]] > cb)
										if(p[pixel[15]] < c_b)
											if(p[pixel[13]] > cb)
												if(p[pixel[7]] > cb)
													if(p[pixel[8]] > cb)
														if(p[pixel[9]] > cb)
															if(p[pixel[10]] > cb)
																if(p[pixel[11]] > cb)
																	if(p[pixel[12]] > cb)
																		if(p[pixel[14]] > cb)
																			goto is_a_corner;
																		else
																			goto is_not_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[13]] < c_b)
												if(p[pixel[14]] < c_b)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[7]] > cb)
											if(p[pixel[8]] > cb)
												if(p[pixel[9]] > cb)
													if(p[pixel[10]] > cb)
														if(p[pixel[11]] > cb)
															if(p[pixel[12]] > cb)
																if(p[pixel[13]] > cb)
																	if(p[pixel[14]] > cb)
																		goto is_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[6]] < c_b)
										if(p[pixel[7]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[8]] > cb)
													if(p[pixel[9]] > cb)
														if(p[pixel[10]] > cb)
															if(p[pixel[11]] > cb)
																if(p[pixel[12]] > cb)
																	if(p[pixel[13]] > cb)
																		if(p[pixel[15]] > cb)
																			goto is_a_corner;
																		else
																			goto is_not_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[7]] < c_b)
											if(p[pixel[8]] < c_b)
												goto is_a_corner;
											else if(p[pixel[15]] < c_b)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[14]] < c_b)
											if(p[pixel[15]] < c_b)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[13]] > cb)
										if(p[pixel[7]] > cb)
											if(p[pixel[8]] > cb)
												if(p[pixel[9]] > cb)
													if(p[pixel[10]] > cb)
														if(p[pixel[11]] > cb)
															if(p[pixel[12]] > cb)
																if(p[pixel[14]] > cb)
																	if(p[pixel[15]] > cb)
																		goto is_a_corner;
																	else
																		goto is_not_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[13]] < c_b)
										if(p[pixel[14]] < c_b)
											if(p[pixel[15]] < c_b)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[12]] > cb)
									if(p[pixel[7]] > cb)
										if(p[pixel[8]] > cb)
											if(p[pixel[9]] > cb)
												if(p[pixel[10]] > cb)
													if(p[pixel[11]] > cb)
														if(p[pixel[13]] > cb)
															if(p[pixel[14]] > cb)
																if(p[pixel[6]] > cb)
																	goto is_a_corner;
																else if(p[pixel[15]] > cb)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[12]] < c_b)
									if(p[pixel[13]] < c_b)
										if(p[pixel[14]] < c_b)
											if(p[pixel[15]] < c_b)
												goto is_a_corner;
											else if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														if(p[pixel[9]] < c_b)
															if(p[pixel[10]] < c_b)
																if(p[pixel[11]] < c_b)
																	goto is_a_corner;
																else
																	goto is_not_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else if(p[pixel[11]] > cb)
								if(p[pixel[7]] > cb)
									if(p[pixel[8]] > cb)
										if(p[pixel[9]] > cb)
											if(p[pixel[10]] > cb)
												if(p[pixel[12]] > cb)
													if(p[pixel[13]] > cb)
														if(p[pixel[6]] > cb)
															if(p[pixel[5]] > cb)
																goto is_a_corner;
															else if(p[pixel[14]] > cb)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else if(p[pixel[14]] > cb)
															if(p[pixel[15]] > cb)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else if(p[pixel[11]] < c_b)
								if(p[pixel[12]] < c_b)
									if(p[pixel[13]] < c_b)
										if(p[pixel[14]] < c_b)
											if(p[pixel[15]] < c_b)
												goto is_a_corner;
											else if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														if(p[pixel[9]] < c_b)
															if(p[pixel[10]] < c_b)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[5]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														if(p[pixel[9]] < c_b)
															if(p[pixel[10]] < c_b)
																goto is_a_corner;
															else
																goto is_not_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else if(p[pixel[10]] > cb)
							if(p[pixel[7]] > cb)
								if(p[pixel[8]] > cb)
									if(p[pixel[9]] > cb)
										if(p[pixel[11]] > cb)
											if(p[pixel[12]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[5]] > cb)
														if(p[pixel[4]] > cb)
															goto is_a_corner;
														else if(p[pixel[13]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else if(p[pixel[13]] > cb)
														if(p[pixel[14]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else if(p[pixel[13]] > cb)
													if(p[pixel[14]] > cb)
														if(p[pixel[15]] > cb)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else if(p[pixel[10]] < c_b)
							if(p[pixel[11]] < c_b)
								if(p[pixel[12]] < c_b)
									if(p[pixel[13]] < c_b)
										if(p[pixel[14]] < c_b)
											if(p[pixel[15]] < c_b)
												goto is_a_corner;
											else if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														if(p[pixel[9]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[5]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														if(p[pixel[9]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[4]] < c_b)
										if(p[pixel[5]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														if(p[pixel[9]] < c_b)
															goto is_a_corner;
														else
															goto is_not_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else if(p[pixel[9]] > cb)
						if(p[pixel[7]] > cb)
							if(p[pixel[8]] > cb)
								if(p[pixel[10]] > cb)
									if(p[pixel[11]] > cb)
										if(p[pixel[6]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[4]] > cb)
													if(p[pixel[3]] > cb)
														goto is_a_corner;
													else if(p[pixel[12]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else if(p[pixel[12]] > cb)
													if(p[pixel[13]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[12]] > cb)
												if(p[pixel[13]] > cb)
													if(p[pixel[14]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[12]] > cb)
											if(p[pixel[13]] > cb)
												if(p[pixel[14]] > cb)
													if(p[pixel[15]] > cb)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else if(p[pixel[9]] < c_b)
						if(p[pixel[10]] < c_b)
							if(p[pixel[11]] < c_b)
								if(p[pixel[12]] < c_b)
									if(p[pixel[13]] < c_b)
										if(p[pixel[14]] < c_b)
											if(p[pixel[15]] < c_b)
												goto is_a_corner;
											else if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[5]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[4]] < c_b)
										if(p[pixel[5]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[3]] < c_b)
									if(p[pixel[4]] < c_b)
										if(p[pixel[5]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													if(p[pixel[8]] < c_b)
														goto is_a_corner;
													else
														goto is_not_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else
						goto is_not_a_corner;
				else if(p[pixel[8]] > cb)
					if(p[pixel[7]] > cb)
						if(p[pixel[9]] > cb)
							if(p[pixel[10]] > cb)
								if(p[pixel[6]] > cb)
									if(p[pixel[5]] > cb)
										if(p[pixel[4]] > cb)
											if(p[pixel[3]] > cb)
												if(p[pixel[2]] > cb)
													goto is_a_corner;
												else if(p[pixel[11]] > cb)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else if(p[pixel[11]] > cb)
												if(p[pixel[12]] > cb)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[11]] > cb)
											if(p[pixel[12]] > cb)
												if(p[pixel[13]] > cb)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[11]] > cb)
										if(p[pixel[12]] > cb)
											if(p[pixel[13]] > cb)
												if(p[pixel[14]] > cb)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[11]] > cb)
									if(p[pixel[12]] > cb)
										if(p[pixel[13]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else
						goto is_not_a_corner;
				else if(p[pixel[8]] < c_b)
					if(p[pixel[9]] < c_b)
						if(p[pixel[10]] < c_b)
							if(p[pixel[11]] < c_b)
								if(p[pixel[12]] < c_b)
									if(p[pixel[13]] < c_b)
										if(p[pixel[14]] < c_b)
											if(p[pixel[15]] < c_b)
												goto is_a_corner;
											else if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[5]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[4]] < c_b)
										if(p[pixel[5]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[3]] < c_b)
									if(p[pixel[4]] < c_b)
										if(p[pixel[5]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else if(p[pixel[2]] < c_b)
								if(p[pixel[3]] < c_b)
									if(p[pixel[4]] < c_b)
										if(p[pixel[5]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[7]] < c_b)
													goto is_a_corner;
												else
													goto is_not_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else
						goto is_not_a_corner;
				else
					goto is_not_a_corner;
			else if(p[pixel[7]] > cb)
				if(p[pixel[8]] > cb)
					if(p[pixel[9]] > cb)
						if(p[pixel[6]] > cb)
							if(p[pixel[5]] > cb)
								if(p[pixel[4]] > cb)
									if(p[pixel[3]] > cb)
										if(p[pixel[2]] > cb)
											if(p[pixel[1]] > cb)
												goto is_a_corner;
											else if(p[pixel[10]] > cb)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[10]] > cb)
											if(p[pixel[11]] > cb)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[10]] > cb)
										if(p[pixel[11]] > cb)
											if(p[pixel[12]] > cb)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[10]] > cb)
									if(p[pixel[11]] > cb)
										if(p[pixel[12]] > cb)
											if(p[pixel[13]] > cb)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else if(p[pixel[10]] > cb)
								if(p[pixel[11]] > cb)
									if(p[pixel[12]] > cb)
										if(p[pixel[13]] > cb)
											if(p[pixel[14]] > cb)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else if(p[pixel[10]] > cb)
							if(p[pixel[11]] > cb)
								if(p[pixel[12]] > cb)
									if(p[pixel[13]] > cb)
										if(p[pixel[14]] > cb)
											if(p[pixel[15]] > cb)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else
						goto is_not_a_corner;
				else
					goto is_not_a_corner;
			else if(p[pixel[7]] < c_b)
				if(p[pixel[8]] < c_b)
					if(p[pixel[9]] < c_b)
						if(p[pixel[6]] < c_b)
							if(p[pixel[5]] < c_b)
								if(p[pixel[4]] < c_b)
									if(p[pixel[3]] < c_b)
										if(p[pixel[2]] < c_b)
											if(p[pixel[1]] < c_b)
												goto is_a_corner;
											else if(p[pixel[10]] < c_b)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else if(p[pixel[10]] < c_b)
											if(p[pixel[11]] < c_b)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else if(p[pixel[10]] < c_b)
										if(p[pixel[11]] < c_b)
											if(p[pixel[12]] < c_b)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else if(p[pixel[10]] < c_b)
									if(p[pixel[11]] < c_b)
										if(p[pixel[12]] < c_b)
											if(p[pixel[13]] < c_b)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else if(p[pixel[10]] < c_b)
								if(p[pixel[11]] < c_b)
									if(p[pixel[12]] < c_b)
										if(p[pixel[13]] < c_b)
											if(p[pixel[14]] < c_b)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else if(p[pixel[10]] < c_b)
							if(p[pixel[11]] < c_b)
								if(p[pixel[12]] < c_b)
									if(p[pixel[13]] < c_b)
										if(p[pixel[14]] < c_b)
											if(p[pixel[15]] < c_b)
												goto is_a_corner;
											else
												goto is_not_a_corner;
										else
											goto is_not_a_corner;
									else
										goto is_not_a_corner;
								else
									goto is_not_a_corner;
							else
								goto is_not_a_corner;
						else
							goto is_not_a_corner;
					else
						goto is_not_a_corner;
				else
					goto is_not_a_corner;
			else
				goto is_not_a_corner;

is_a_corner:
			bmin = b;
			goto end_if;
is_not_a_corner:
			bmax = b;
			goto end_if;
end_if:

			if(bmin == bmax - 1 || bmin == bmax)
				return bmin;

			b = (bmin + bmax) >> 1;
		}
	}

	void cornersScores(const unsigned char* i, int stride, unsigned char b) {
		scores.resize(ret_corners.size());
		int n;
		int pixel[16];
		makeOffsets(pixel, stride);

		for(n = 0; n < (int) ret_corners.size(); ++n)
			scores[n] = cornerScore(i + ret_corners[n].y * stride + ret_corners[n].x, pixel, b);
	}

	void detectAllCorners(const unsigned char* im, int xsize, int ysize, int stride, unsigned char b) {
		ret_corners.clear();
		int pixel[16];
		makeOffsets(pixel, stride);
		F9_CORNER xy;

		for(xy.y = 3; xy.y < ysize - 3; ++xy.y)
			for(xy.x = 3; xy.x < xsize - 3; ++xy.x) {
				const unsigned char* p = im + xy.y * stride + xy.x;
				const unsigned char cb((*p > 255 - b) ? 255 : *p + b);
				const unsigned char c_b((*p < b) ? 0 : *p - b);

				if(p[pixel[0]] > cb)
					if(p[pixel[1]] > cb)
						if(p[pixel[2]] > cb)
							if(p[pixel[3]] > cb)
								if(p[pixel[4]] > cb)
									if(p[pixel[5]] > cb)
										if(p[pixel[6]] > cb)
											if(p[pixel[7]] > cb)
												if(p[pixel[8]] > cb) {
												}
												else if(p[pixel[15]] > cb) {
												}
												else
													continue;
											else if(p[pixel[7]] < c_b)
												if(p[pixel[14]] > cb)
													if(p[pixel[15]] > cb) {
													}
													else
														continue;
												else if(p[pixel[14]] < c_b)
													if(p[pixel[8]] < c_b)
														if(p[pixel[9]] < c_b)
															if(p[pixel[10]] < c_b)
																if(p[pixel[11]] < c_b)
																	if(p[pixel[12]] < c_b)
																		if(p[pixel[13]] < c_b)
																			if(p[pixel[15]] < c_b) {
																			}
																			else
																				continue;
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb) {
												}
												else
													continue;
											else
												continue;
										else if(p[pixel[6]] < c_b)
											if(p[pixel[15]] > cb)
												if(p[pixel[13]] > cb)
													if(p[pixel[14]] > cb) {
													}
													else
														continue;
												else if(p[pixel[13]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b)
																if(p[pixel[10]] < c_b)
																	if(p[pixel[11]] < c_b)
																		if(p[pixel[12]] < c_b)
																			if(p[pixel[14]] < c_b) {
																			}
																			else
																				continue;
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[7]] < c_b)
												if(p[pixel[8]] < c_b)
													if(p[pixel[9]] < c_b)
														if(p[pixel[10]] < c_b)
															if(p[pixel[11]] < c_b)
																if(p[pixel[12]] < c_b)
																	if(p[pixel[13]] < c_b)
																		if(p[pixel[14]] < c_b) {
																		}
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[13]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb) {
												}
												else
													continue;
											else
												continue;
										else if(p[pixel[13]] < c_b)
											if(p[pixel[7]] < c_b)
												if(p[pixel[8]] < c_b)
													if(p[pixel[9]] < c_b)
														if(p[pixel[10]] < c_b)
															if(p[pixel[11]] < c_b)
																if(p[pixel[12]] < c_b)
																	if(p[pixel[14]] < c_b)
																		if(p[pixel[15]] < c_b) {
																		}
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[5]] < c_b)
										if(p[pixel[14]] > cb)
											if(p[pixel[12]] > cb)
												if(p[pixel[13]] > cb)
													if(p[pixel[15]] > cb) {
													}
													else if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb)
															if(p[pixel[8]] > cb)
																if(p[pixel[9]] > cb)
																	if(p[pixel[10]] > cb)
																		if(p[pixel[11]] > cb) {
																		}
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[12]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b)
																if(p[pixel[10]] < c_b)
																	if(p[pixel[11]] < c_b)
																		if(p[pixel[13]] < c_b) {
																		}
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[14]] < c_b)
											if(p[pixel[7]] < c_b)
												if(p[pixel[8]] < c_b)
													if(p[pixel[9]] < c_b)
														if(p[pixel[10]] < c_b)
															if(p[pixel[11]] < c_b)
																if(p[pixel[12]] < c_b)
																	if(p[pixel[13]] < c_b)
																		if(p[pixel[6]] < c_b) {
																		}
																		else if(p[pixel[15]] < c_b) {
																		}
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[6]] < c_b)
											if(p[pixel[7]] < c_b)
												if(p[pixel[8]] < c_b)
													if(p[pixel[9]] < c_b)
														if(p[pixel[10]] < c_b)
															if(p[pixel[11]] < c_b)
																if(p[pixel[12]] < c_b)
																	if(p[pixel[13]] < c_b) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[12]] > cb)
										if(p[pixel[13]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb) {
												}
												else if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb)
																if(p[pixel[10]] > cb)
																	if(p[pixel[11]] > cb) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[12]] < c_b)
										if(p[pixel[7]] < c_b)
											if(p[pixel[8]] < c_b)
												if(p[pixel[9]] < c_b)
													if(p[pixel[10]] < c_b)
														if(p[pixel[11]] < c_b)
															if(p[pixel[13]] < c_b)
																if(p[pixel[14]] < c_b)
																	if(p[pixel[6]] < c_b) {
																	}
																	else if(p[pixel[15]] < c_b) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else if(p[pixel[4]] < c_b)
									if(p[pixel[13]] > cb)
										if(p[pixel[11]] > cb)
											if(p[pixel[12]] > cb)
												if(p[pixel[14]] > cb)
													if(p[pixel[15]] > cb) {
													}
													else if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb)
															if(p[pixel[8]] > cb)
																if(p[pixel[9]] > cb)
																	if(p[pixel[10]] > cb) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else if(p[pixel[5]] > cb)
													if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb)
															if(p[pixel[8]] > cb)
																if(p[pixel[9]] > cb)
																	if(p[pixel[10]] > cb) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[11]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b)
																if(p[pixel[10]] < c_b)
																	if(p[pixel[12]] < c_b) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[13]] < c_b)
										if(p[pixel[7]] < c_b)
											if(p[pixel[8]] < c_b)
												if(p[pixel[9]] < c_b)
													if(p[pixel[10]] < c_b)
														if(p[pixel[11]] < c_b)
															if(p[pixel[12]] < c_b)
																if(p[pixel[6]] < c_b)
																	if(p[pixel[5]] < c_b) {
																	}
																	else if(p[pixel[14]] < c_b) {
																	}
																	else
																		continue;
																else if(p[pixel[14]] < c_b)
																	if(p[pixel[15]] < c_b) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[5]] < c_b)
										if(p[pixel[6]] < c_b)
											if(p[pixel[7]] < c_b)
												if(p[pixel[8]] < c_b)
													if(p[pixel[9]] < c_b)
														if(p[pixel[10]] < c_b)
															if(p[pixel[11]] < c_b)
																if(p[pixel[12]] < c_b) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else if(p[pixel[11]] > cb)
									if(p[pixel[12]] > cb)
										if(p[pixel[13]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb) {
												}
												else if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb)
																if(p[pixel[10]] > cb) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb)
																if(p[pixel[10]] > cb) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else if(p[pixel[11]] < c_b)
									if(p[pixel[7]] < c_b)
										if(p[pixel[8]] < c_b)
											if(p[pixel[9]] < c_b)
												if(p[pixel[10]] < c_b)
													if(p[pixel[12]] < c_b)
														if(p[pixel[13]] < c_b)
															if(p[pixel[6]] < c_b)
																if(p[pixel[5]] < c_b) {
																}
																else if(p[pixel[14]] < c_b) {
																}
																else
																	continue;
															else if(p[pixel[14]] < c_b)
																if(p[pixel[15]] < c_b) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else if(p[pixel[3]] < c_b)
								if(p[pixel[10]] > cb)
									if(p[pixel[11]] > cb)
										if(p[pixel[12]] > cb)
											if(p[pixel[13]] > cb)
												if(p[pixel[14]] > cb)
													if(p[pixel[15]] > cb) {
													}
													else if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb)
															if(p[pixel[8]] > cb)
																if(p[pixel[9]] > cb) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else if(p[pixel[5]] > cb)
													if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb)
															if(p[pixel[8]] > cb)
																if(p[pixel[9]] > cb) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[4]] > cb)
												if(p[pixel[5]] > cb)
													if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb)
															if(p[pixel[8]] > cb)
																if(p[pixel[9]] > cb) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else if(p[pixel[10]] < c_b)
									if(p[pixel[7]] < c_b)
										if(p[pixel[8]] < c_b)
											if(p[pixel[9]] < c_b)
												if(p[pixel[11]] < c_b)
													if(p[pixel[6]] < c_b)
														if(p[pixel[5]] < c_b)
															if(p[pixel[4]] < c_b) {
															}
															else if(p[pixel[12]] < c_b)
																if(p[pixel[13]] < c_b) {
																}
																else
																	continue;
															else
																continue;
														else if(p[pixel[12]] < c_b)
															if(p[pixel[13]] < c_b)
																if(p[pixel[14]] < c_b) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else if(p[pixel[12]] < c_b)
														if(p[pixel[13]] < c_b)
															if(p[pixel[14]] < c_b)
																if(p[pixel[15]] < c_b) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else if(p[pixel[10]] > cb)
								if(p[pixel[11]] > cb)
									if(p[pixel[12]] > cb)
										if(p[pixel[13]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb) {
												}
												else if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[4]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else if(p[pixel[10]] < c_b)
								if(p[pixel[7]] < c_b)
									if(p[pixel[8]] < c_b)
										if(p[pixel[9]] < c_b)
											if(p[pixel[11]] < c_b)
												if(p[pixel[12]] < c_b)
													if(p[pixel[6]] < c_b)
														if(p[pixel[5]] < c_b)
															if(p[pixel[4]] < c_b) {
															}
															else if(p[pixel[13]] < c_b) {
															}
															else
																continue;
														else if(p[pixel[13]] < c_b)
															if(p[pixel[14]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else if(p[pixel[13]] < c_b)
														if(p[pixel[14]] < c_b)
															if(p[pixel[15]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else if(p[pixel[2]] < c_b)
							if(p[pixel[9]] > cb)
								if(p[pixel[10]] > cb)
									if(p[pixel[11]] > cb)
										if(p[pixel[12]] > cb)
											if(p[pixel[13]] > cb)
												if(p[pixel[14]] > cb)
													if(p[pixel[15]] > cb) {
													}
													else if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb)
															if(p[pixel[8]] > cb) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else if(p[pixel[5]] > cb)
													if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb)
															if(p[pixel[8]] > cb) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[4]] > cb)
												if(p[pixel[5]] > cb)
													if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb)
															if(p[pixel[8]] > cb) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[3]] > cb)
											if(p[pixel[4]] > cb)
												if(p[pixel[5]] > cb)
													if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb)
															if(p[pixel[8]] > cb) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else if(p[pixel[9]] < c_b)
								if(p[pixel[7]] < c_b)
									if(p[pixel[8]] < c_b)
										if(p[pixel[10]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[5]] < c_b)
													if(p[pixel[4]] < c_b)
														if(p[pixel[3]] < c_b) {
														}
														else if(p[pixel[11]] < c_b)
															if(p[pixel[12]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else if(p[pixel[11]] < c_b)
														if(p[pixel[12]] < c_b)
															if(p[pixel[13]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else if(p[pixel[11]] < c_b)
													if(p[pixel[12]] < c_b)
														if(p[pixel[13]] < c_b)
															if(p[pixel[14]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[11]] < c_b)
												if(p[pixel[12]] < c_b)
													if(p[pixel[13]] < c_b)
														if(p[pixel[14]] < c_b)
															if(p[pixel[15]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else if(p[pixel[9]] > cb)
							if(p[pixel[10]] > cb)
								if(p[pixel[11]] > cb)
									if(p[pixel[12]] > cb)
										if(p[pixel[13]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb) {
												}
												else if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[4]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[3]] > cb)
										if(p[pixel[4]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else if(p[pixel[9]] < c_b)
							if(p[pixel[7]] < c_b)
								if(p[pixel[8]] < c_b)
									if(p[pixel[10]] < c_b)
										if(p[pixel[11]] < c_b)
											if(p[pixel[6]] < c_b)
												if(p[pixel[5]] < c_b)
													if(p[pixel[4]] < c_b)
														if(p[pixel[3]] < c_b) {
														}
														else if(p[pixel[12]] < c_b) {
														}
														else
															continue;
													else if(p[pixel[12]] < c_b)
														if(p[pixel[13]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else if(p[pixel[12]] < c_b)
													if(p[pixel[13]] < c_b)
														if(p[pixel[14]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[12]] < c_b)
												if(p[pixel[13]] < c_b)
													if(p[pixel[14]] < c_b)
														if(p[pixel[15]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else
							continue;
					else if(p[pixel[1]] < c_b)
						if(p[pixel[8]] > cb)
							if(p[pixel[9]] > cb)
								if(p[pixel[10]] > cb)
									if(p[pixel[11]] > cb)
										if(p[pixel[12]] > cb)
											if(p[pixel[13]] > cb)
												if(p[pixel[14]] > cb)
													if(p[pixel[15]] > cb) {
													}
													else if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb) {
														}
														else
															continue;
													else
														continue;
												else if(p[pixel[5]] > cb)
													if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[4]] > cb)
												if(p[pixel[5]] > cb)
													if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[3]] > cb)
											if(p[pixel[4]] > cb)
												if(p[pixel[5]] > cb)
													if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[2]] > cb)
										if(p[pixel[3]] > cb)
											if(p[pixel[4]] > cb)
												if(p[pixel[5]] > cb)
													if(p[pixel[6]] > cb)
														if(p[pixel[7]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else if(p[pixel[8]] < c_b)
							if(p[pixel[7]] < c_b)
								if(p[pixel[9]] < c_b)
									if(p[pixel[6]] < c_b)
										if(p[pixel[5]] < c_b)
											if(p[pixel[4]] < c_b)
												if(p[pixel[3]] < c_b)
													if(p[pixel[2]] < c_b) {
													}
													else if(p[pixel[10]] < c_b)
														if(p[pixel[11]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else if(p[pixel[10]] < c_b)
													if(p[pixel[11]] < c_b)
														if(p[pixel[12]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[10]] < c_b)
												if(p[pixel[11]] < c_b)
													if(p[pixel[12]] < c_b)
														if(p[pixel[13]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[10]] < c_b)
											if(p[pixel[11]] < c_b)
												if(p[pixel[12]] < c_b)
													if(p[pixel[13]] < c_b)
														if(p[pixel[14]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[10]] < c_b)
										if(p[pixel[11]] < c_b)
											if(p[pixel[12]] < c_b)
												if(p[pixel[13]] < c_b)
													if(p[pixel[14]] < c_b)
														if(p[pixel[15]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else
							continue;
					else if(p[pixel[8]] > cb)
						if(p[pixel[9]] > cb)
							if(p[pixel[10]] > cb)
								if(p[pixel[11]] > cb)
									if(p[pixel[12]] > cb)
										if(p[pixel[13]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb) {
												}
												else if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb) {
													}
													else
														continue;
												else
													continue;
											else if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[4]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[3]] > cb)
										if(p[pixel[4]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else if(p[pixel[2]] > cb)
									if(p[pixel[3]] > cb)
										if(p[pixel[4]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else
							continue;
					else if(p[pixel[8]] < c_b)
						if(p[pixel[7]] < c_b)
							if(p[pixel[9]] < c_b)
								if(p[pixel[10]] < c_b)
									if(p[pixel[6]] < c_b)
										if(p[pixel[5]] < c_b)
											if(p[pixel[4]] < c_b)
												if(p[pixel[3]] < c_b)
													if(p[pixel[2]] < c_b) {
													}
													else if(p[pixel[11]] < c_b) {
													}
													else
														continue;
												else if(p[pixel[11]] < c_b)
													if(p[pixel[12]] < c_b) {
													}
													else
														continue;
												else
													continue;
											else if(p[pixel[11]] < c_b)
												if(p[pixel[12]] < c_b)
													if(p[pixel[13]] < c_b) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[11]] < c_b)
											if(p[pixel[12]] < c_b)
												if(p[pixel[13]] < c_b)
													if(p[pixel[14]] < c_b) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[11]] < c_b)
										if(p[pixel[12]] < c_b)
											if(p[pixel[13]] < c_b)
												if(p[pixel[14]] < c_b)
													if(p[pixel[15]] < c_b) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else
							continue;
					else
						continue;
				else if(p[pixel[0]] < c_b)
					if(p[pixel[1]] > cb)
						if(p[pixel[8]] > cb)
							if(p[pixel[7]] > cb)
								if(p[pixel[9]] > cb)
									if(p[pixel[6]] > cb)
										if(p[pixel[5]] > cb)
											if(p[pixel[4]] > cb)
												if(p[pixel[3]] > cb)
													if(p[pixel[2]] > cb) {
													}
													else if(p[pixel[10]] > cb)
														if(p[pixel[11]] > cb) {
														}
														else
															continue;
													else
														continue;
												else if(p[pixel[10]] > cb)
													if(p[pixel[11]] > cb)
														if(p[pixel[12]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[10]] > cb)
												if(p[pixel[11]] > cb)
													if(p[pixel[12]] > cb)
														if(p[pixel[13]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[10]] > cb)
											if(p[pixel[11]] > cb)
												if(p[pixel[12]] > cb)
													if(p[pixel[13]] > cb)
														if(p[pixel[14]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[10]] > cb)
										if(p[pixel[11]] > cb)
											if(p[pixel[12]] > cb)
												if(p[pixel[13]] > cb)
													if(p[pixel[14]] > cb)
														if(p[pixel[15]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else if(p[pixel[8]] < c_b)
							if(p[pixel[9]] < c_b)
								if(p[pixel[10]] < c_b)
									if(p[pixel[11]] < c_b)
										if(p[pixel[12]] < c_b)
											if(p[pixel[13]] < c_b)
												if(p[pixel[14]] < c_b)
													if(p[pixel[15]] < c_b) {
													}
													else if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else if(p[pixel[5]] < c_b)
													if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[4]] < c_b)
												if(p[pixel[5]] < c_b)
													if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[3]] < c_b)
											if(p[pixel[4]] < c_b)
												if(p[pixel[5]] < c_b)
													if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[2]] < c_b)
										if(p[pixel[3]] < c_b)
											if(p[pixel[4]] < c_b)
												if(p[pixel[5]] < c_b)
													if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else
							continue;
					else if(p[pixel[1]] < c_b)
						if(p[pixel[2]] > cb)
							if(p[pixel[9]] > cb)
								if(p[pixel[7]] > cb)
									if(p[pixel[8]] > cb)
										if(p[pixel[10]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[5]] > cb)
													if(p[pixel[4]] > cb)
														if(p[pixel[3]] > cb) {
														}
														else if(p[pixel[11]] > cb)
															if(p[pixel[12]] > cb) {
															}
															else
																continue;
														else
															continue;
													else if(p[pixel[11]] > cb)
														if(p[pixel[12]] > cb)
															if(p[pixel[13]] > cb) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else if(p[pixel[11]] > cb)
													if(p[pixel[12]] > cb)
														if(p[pixel[13]] > cb)
															if(p[pixel[14]] > cb) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[11]] > cb)
												if(p[pixel[12]] > cb)
													if(p[pixel[13]] > cb)
														if(p[pixel[14]] > cb)
															if(p[pixel[15]] > cb) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else if(p[pixel[9]] < c_b)
								if(p[pixel[10]] < c_b)
									if(p[pixel[11]] < c_b)
										if(p[pixel[12]] < c_b)
											if(p[pixel[13]] < c_b)
												if(p[pixel[14]] < c_b)
													if(p[pixel[15]] < c_b) {
													}
													else if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b)
															if(p[pixel[8]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else if(p[pixel[5]] < c_b)
													if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b)
															if(p[pixel[8]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[4]] < c_b)
												if(p[pixel[5]] < c_b)
													if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b)
															if(p[pixel[8]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[3]] < c_b)
											if(p[pixel[4]] < c_b)
												if(p[pixel[5]] < c_b)
													if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b)
															if(p[pixel[8]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else if(p[pixel[2]] < c_b)
							if(p[pixel[3]] > cb)
								if(p[pixel[10]] > cb)
									if(p[pixel[7]] > cb)
										if(p[pixel[8]] > cb)
											if(p[pixel[9]] > cb)
												if(p[pixel[11]] > cb)
													if(p[pixel[6]] > cb)
														if(p[pixel[5]] > cb)
															if(p[pixel[4]] > cb) {
															}
															else if(p[pixel[12]] > cb)
																if(p[pixel[13]] > cb) {
																}
																else
																	continue;
															else
																continue;
														else if(p[pixel[12]] > cb)
															if(p[pixel[13]] > cb)
																if(p[pixel[14]] > cb) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else if(p[pixel[12]] > cb)
														if(p[pixel[13]] > cb)
															if(p[pixel[14]] > cb)
																if(p[pixel[15]] > cb) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else if(p[pixel[10]] < c_b)
									if(p[pixel[11]] < c_b)
										if(p[pixel[12]] < c_b)
											if(p[pixel[13]] < c_b)
												if(p[pixel[14]] < c_b)
													if(p[pixel[15]] < c_b) {
													}
													else if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b)
															if(p[pixel[8]] < c_b)
																if(p[pixel[9]] < c_b) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else if(p[pixel[5]] < c_b)
													if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b)
															if(p[pixel[8]] < c_b)
																if(p[pixel[9]] < c_b) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[4]] < c_b)
												if(p[pixel[5]] < c_b)
													if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b)
															if(p[pixel[8]] < c_b)
																if(p[pixel[9]] < c_b) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else if(p[pixel[3]] < c_b)
								if(p[pixel[4]] > cb)
									if(p[pixel[13]] > cb)
										if(p[pixel[7]] > cb)
											if(p[pixel[8]] > cb)
												if(p[pixel[9]] > cb)
													if(p[pixel[10]] > cb)
														if(p[pixel[11]] > cb)
															if(p[pixel[12]] > cb)
																if(p[pixel[6]] > cb)
																	if(p[pixel[5]] > cb) {
																	}
																	else if(p[pixel[14]] > cb) {
																	}
																	else
																		continue;
																else if(p[pixel[14]] > cb)
																	if(p[pixel[15]] > cb) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[13]] < c_b)
										if(p[pixel[11]] > cb)
											if(p[pixel[5]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb)
																if(p[pixel[10]] > cb)
																	if(p[pixel[12]] > cb) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[11]] < c_b)
											if(p[pixel[12]] < c_b)
												if(p[pixel[14]] < c_b)
													if(p[pixel[15]] < c_b) {
													}
													else if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b)
															if(p[pixel[8]] < c_b)
																if(p[pixel[9]] < c_b)
																	if(p[pixel[10]] < c_b) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else if(p[pixel[5]] < c_b)
													if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b)
															if(p[pixel[8]] < c_b)
																if(p[pixel[9]] < c_b)
																	if(p[pixel[10]] < c_b) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[5]] > cb)
										if(p[pixel[6]] > cb)
											if(p[pixel[7]] > cb)
												if(p[pixel[8]] > cb)
													if(p[pixel[9]] > cb)
														if(p[pixel[10]] > cb)
															if(p[pixel[11]] > cb)
																if(p[pixel[12]] > cb) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else if(p[pixel[4]] < c_b)
									if(p[pixel[5]] > cb)
										if(p[pixel[14]] > cb)
											if(p[pixel[7]] > cb)
												if(p[pixel[8]] > cb)
													if(p[pixel[9]] > cb)
														if(p[pixel[10]] > cb)
															if(p[pixel[11]] > cb)
																if(p[pixel[12]] > cb)
																	if(p[pixel[13]] > cb)
																		if(p[pixel[6]] > cb) {
																		}
																		else if(p[pixel[15]] > cb) {
																		}
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[14]] < c_b)
											if(p[pixel[12]] > cb)
												if(p[pixel[6]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb)
																if(p[pixel[10]] > cb)
																	if(p[pixel[11]] > cb)
																		if(p[pixel[13]] > cb) {
																		}
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[12]] < c_b)
												if(p[pixel[13]] < c_b)
													if(p[pixel[15]] < c_b) {
													}
													else if(p[pixel[6]] < c_b)
														if(p[pixel[7]] < c_b)
															if(p[pixel[8]] < c_b)
																if(p[pixel[9]] < c_b)
																	if(p[pixel[10]] < c_b)
																		if(p[pixel[11]] < c_b) {
																		}
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[6]] > cb)
											if(p[pixel[7]] > cb)
												if(p[pixel[8]] > cb)
													if(p[pixel[9]] > cb)
														if(p[pixel[10]] > cb)
															if(p[pixel[11]] > cb)
																if(p[pixel[12]] > cb)
																	if(p[pixel[13]] > cb) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[5]] < c_b)
										if(p[pixel[6]] > cb)
											if(p[pixel[15]] < c_b)
												if(p[pixel[13]] > cb)
													if(p[pixel[7]] > cb)
														if(p[pixel[8]] > cb)
															if(p[pixel[9]] > cb)
																if(p[pixel[10]] > cb)
																	if(p[pixel[11]] > cb)
																		if(p[pixel[12]] > cb)
																			if(p[pixel[14]] > cb) {
																			}
																			else
																				continue;
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else if(p[pixel[13]] < c_b)
													if(p[pixel[14]] < c_b) {
													}
													else
														continue;
												else
													continue;
											else if(p[pixel[7]] > cb)
												if(p[pixel[8]] > cb)
													if(p[pixel[9]] > cb)
														if(p[pixel[10]] > cb)
															if(p[pixel[11]] > cb)
																if(p[pixel[12]] > cb)
																	if(p[pixel[13]] > cb)
																		if(p[pixel[14]] > cb) {
																		}
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[6]] < c_b)
											if(p[pixel[7]] > cb)
												if(p[pixel[14]] > cb)
													if(p[pixel[8]] > cb)
														if(p[pixel[9]] > cb)
															if(p[pixel[10]] > cb)
																if(p[pixel[11]] > cb)
																	if(p[pixel[12]] > cb)
																		if(p[pixel[13]] > cb)
																			if(p[pixel[15]] > cb) {
																			}
																			else
																				continue;
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else if(p[pixel[14]] < c_b)
													if(p[pixel[15]] < c_b) {
													}
													else
														continue;
												else
													continue;
											else if(p[pixel[7]] < c_b)
												if(p[pixel[8]] < c_b) {
												}
												else if(p[pixel[15]] < c_b) {
												}
												else
													continue;
											else if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b) {
												}
												else
													continue;
											else
												continue;
										else if(p[pixel[13]] > cb)
											if(p[pixel[7]] > cb)
												if(p[pixel[8]] > cb)
													if(p[pixel[9]] > cb)
														if(p[pixel[10]] > cb)
															if(p[pixel[11]] > cb)
																if(p[pixel[12]] > cb)
																	if(p[pixel[14]] > cb)
																		if(p[pixel[15]] > cb) {
																		}
																		else
																			continue;
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[13]] < c_b)
											if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b) {
												}
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[12]] > cb)
										if(p[pixel[7]] > cb)
											if(p[pixel[8]] > cb)
												if(p[pixel[9]] > cb)
													if(p[pixel[10]] > cb)
														if(p[pixel[11]] > cb)
															if(p[pixel[13]] > cb)
																if(p[pixel[14]] > cb)
																	if(p[pixel[6]] > cb) {
																	}
																	else if(p[pixel[15]] > cb) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[12]] < c_b)
										if(p[pixel[13]] < c_b)
											if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b) {
												}
												else if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b)
																if(p[pixel[10]] < c_b)
																	if(p[pixel[11]] < c_b) {
																	}
																	else
																		continue;
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else if(p[pixel[11]] > cb)
									if(p[pixel[7]] > cb)
										if(p[pixel[8]] > cb)
											if(p[pixel[9]] > cb)
												if(p[pixel[10]] > cb)
													if(p[pixel[12]] > cb)
														if(p[pixel[13]] > cb)
															if(p[pixel[6]] > cb)
																if(p[pixel[5]] > cb) {
																}
																else if(p[pixel[14]] > cb) {
																}
																else
																	continue;
															else if(p[pixel[14]] > cb)
																if(p[pixel[15]] > cb) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else if(p[pixel[11]] < c_b)
									if(p[pixel[12]] < c_b)
										if(p[pixel[13]] < c_b)
											if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b) {
												}
												else if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b)
																if(p[pixel[10]] < c_b) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b)
																if(p[pixel[10]] < c_b) {
																}
																else
																	continue;
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else if(p[pixel[10]] > cb)
								if(p[pixel[7]] > cb)
									if(p[pixel[8]] > cb)
										if(p[pixel[9]] > cb)
											if(p[pixel[11]] > cb)
												if(p[pixel[12]] > cb)
													if(p[pixel[6]] > cb)
														if(p[pixel[5]] > cb)
															if(p[pixel[4]] > cb) {
															}
															else if(p[pixel[13]] > cb) {
															}
															else
																continue;
														else if(p[pixel[13]] > cb)
															if(p[pixel[14]] > cb) {
															}
															else
																continue;
														else
															continue;
													else if(p[pixel[13]] > cb)
														if(p[pixel[14]] > cb)
															if(p[pixel[15]] > cb) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else if(p[pixel[10]] < c_b)
								if(p[pixel[11]] < c_b)
									if(p[pixel[12]] < c_b)
										if(p[pixel[13]] < c_b)
											if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b) {
												}
												else if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[4]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b)
															if(p[pixel[9]] < c_b) {
															}
															else
																continue;
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else if(p[pixel[9]] > cb)
							if(p[pixel[7]] > cb)
								if(p[pixel[8]] > cb)
									if(p[pixel[10]] > cb)
										if(p[pixel[11]] > cb)
											if(p[pixel[6]] > cb)
												if(p[pixel[5]] > cb)
													if(p[pixel[4]] > cb)
														if(p[pixel[3]] > cb) {
														}
														else if(p[pixel[12]] > cb) {
														}
														else
															continue;
													else if(p[pixel[12]] > cb)
														if(p[pixel[13]] > cb) {
														}
														else
															continue;
													else
														continue;
												else if(p[pixel[12]] > cb)
													if(p[pixel[13]] > cb)
														if(p[pixel[14]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[12]] > cb)
												if(p[pixel[13]] > cb)
													if(p[pixel[14]] > cb)
														if(p[pixel[15]] > cb) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else if(p[pixel[9]] < c_b)
							if(p[pixel[10]] < c_b)
								if(p[pixel[11]] < c_b)
									if(p[pixel[12]] < c_b)
										if(p[pixel[13]] < c_b)
											if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b) {
												}
												else if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[4]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[3]] < c_b)
										if(p[pixel[4]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b)
														if(p[pixel[8]] < c_b) {
														}
														else
															continue;
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else
							continue;
					else if(p[pixel[8]] > cb)
						if(p[pixel[7]] > cb)
							if(p[pixel[9]] > cb)
								if(p[pixel[10]] > cb)
									if(p[pixel[6]] > cb)
										if(p[pixel[5]] > cb)
											if(p[pixel[4]] > cb)
												if(p[pixel[3]] > cb)
													if(p[pixel[2]] > cb) {
													}
													else if(p[pixel[11]] > cb) {
													}
													else
														continue;
												else if(p[pixel[11]] > cb)
													if(p[pixel[12]] > cb) {
													}
													else
														continue;
												else
													continue;
											else if(p[pixel[11]] > cb)
												if(p[pixel[12]] > cb)
													if(p[pixel[13]] > cb) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[11]] > cb)
											if(p[pixel[12]] > cb)
												if(p[pixel[13]] > cb)
													if(p[pixel[14]] > cb) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[11]] > cb)
										if(p[pixel[12]] > cb)
											if(p[pixel[13]] > cb)
												if(p[pixel[14]] > cb)
													if(p[pixel[15]] > cb) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else
							continue;
					else if(p[pixel[8]] < c_b)
						if(p[pixel[9]] < c_b)
							if(p[pixel[10]] < c_b)
								if(p[pixel[11]] < c_b)
									if(p[pixel[12]] < c_b)
										if(p[pixel[13]] < c_b)
											if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b) {
												}
												else if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b) {
													}
													else
														continue;
												else
													continue;
											else if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else if(p[pixel[4]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[3]] < c_b)
										if(p[pixel[4]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else if(p[pixel[2]] < c_b)
									if(p[pixel[3]] < c_b)
										if(p[pixel[4]] < c_b)
											if(p[pixel[5]] < c_b)
												if(p[pixel[6]] < c_b)
													if(p[pixel[7]] < c_b) {
													}
													else
														continue;
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else
							continue;
					else
						continue;
				else if(p[pixel[7]] > cb)
					if(p[pixel[8]] > cb)
						if(p[pixel[9]] > cb)
							if(p[pixel[6]] > cb)
								if(p[pixel[5]] > cb)
									if(p[pixel[4]] > cb)
										if(p[pixel[3]] > cb)
											if(p[pixel[2]] > cb)
												if(p[pixel[1]] > cb) {
												}
												else if(p[pixel[10]] > cb) {
												}
												else
													continue;
											else if(p[pixel[10]] > cb)
												if(p[pixel[11]] > cb) {
												}
												else
													continue;
											else
												continue;
										else if(p[pixel[10]] > cb)
											if(p[pixel[11]] > cb)
												if(p[pixel[12]] > cb) {
												}
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[10]] > cb)
										if(p[pixel[11]] > cb)
											if(p[pixel[12]] > cb)
												if(p[pixel[13]] > cb) {
												}
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else if(p[pixel[10]] > cb)
									if(p[pixel[11]] > cb)
										if(p[pixel[12]] > cb)
											if(p[pixel[13]] > cb)
												if(p[pixel[14]] > cb) {
												}
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else if(p[pixel[10]] > cb)
								if(p[pixel[11]] > cb)
									if(p[pixel[12]] > cb)
										if(p[pixel[13]] > cb)
											if(p[pixel[14]] > cb)
												if(p[pixel[15]] > cb) {
												}
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else
							continue;
					else
						continue;
				else if(p[pixel[7]] < c_b)
					if(p[pixel[8]] < c_b)
						if(p[pixel[9]] < c_b)
							if(p[pixel[6]] < c_b)
								if(p[pixel[5]] < c_b)
									if(p[pixel[4]] < c_b)
										if(p[pixel[3]] < c_b)
											if(p[pixel[2]] < c_b)
												if(p[pixel[1]] < c_b) {
												}
												else if(p[pixel[10]] < c_b) {
												}
												else
													continue;
											else if(p[pixel[10]] < c_b)
												if(p[pixel[11]] < c_b) {
												}
												else
													continue;
											else
												continue;
										else if(p[pixel[10]] < c_b)
											if(p[pixel[11]] < c_b)
												if(p[pixel[12]] < c_b) {
												}
												else
													continue;
											else
												continue;
										else
											continue;
									else if(p[pixel[10]] < c_b)
										if(p[pixel[11]] < c_b)
											if(p[pixel[12]] < c_b)
												if(p[pixel[13]] < c_b) {
												}
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else if(p[pixel[10]] < c_b)
									if(p[pixel[11]] < c_b)
										if(p[pixel[12]] < c_b)
											if(p[pixel[13]] < c_b)
												if(p[pixel[14]] < c_b) {
												}
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else if(p[pixel[10]] < c_b)
								if(p[pixel[11]] < c_b)
									if(p[pixel[12]] < c_b)
										if(p[pixel[13]] < c_b)
											if(p[pixel[14]] < c_b)
												if(p[pixel[15]] < c_b) {
												}
												else
													continue;
											else
												continue;
										else
											continue;
									else
										continue;
								else
									continue;
							else
								continue;
						else
							continue;
					else
						continue;
				else
					continue;

				ret_corners.push_back(xy);
			}
	}
}; // Impl

// C++ API
F9::F9(): impl(0) {
	impl = new Impl();
}

F9::~F9() {
	delete impl;
}

F9::operator bool() const {
	return impl;
}

const std::vector<F9_CORNER>& F9::detectCorners(
    const unsigned char* image_data,
    int width,
    int height,
    int bytes_per_row,
    unsigned char threshold,
    bool suppress_non_max
) {
	return impl->detectCorners(
	           image_data,
	           width,
	           height,
	           bytes_per_row,
	           threshold,
	           suppress_non_max
	       );
}

// C API
void* f9_alloc() {
	return reinterpret_cast<void*>(new F9());
}

void f9_dealloc(void* ctx) {
	delete reinterpret_cast<F9*>(ctx);
}

const F9_CORNER* const f9_detect_corners(
    void* ctx,
    const unsigned char* image_data,
    int width,
    int height,
    int bytes_per_row,
    unsigned char threshold,
    bool suppress_non_max,
    int* ret_num_corners
) {
	const std::vector<F9_CORNER>& result = reinterpret_cast<F9*>(ctx)->detectCorners(
	        image_data,
	        width,
	        height,
	        bytes_per_row,
	        threshold,
	        suppress_non_max
	                                       );
	*ret_num_corners = result.size();
	return &result[0];
}
