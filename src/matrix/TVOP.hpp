/*
 *  jrrs Copyright (C) 2007-2010 Kaveh Vahedipour
 *                               Forschungszentrum Juelich, Germany
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301  USA
 */

#ifndef __TVOP_HPP__
#define __TVOP_HPP__

#include "Matrix.hpp"

/**
 * @brief 2D Finite difference operator
 */
class TVOP {
	

public:

	/**
	 * @brief    2D Forward transform
	 *
	 * @param  m To transform
	 * @return   Transform
	 */
	template <class T> static Matrix<T> 
	Transform (const Matrix<T>& m) {
		
		size_t M = m.Dim(0);
		size_t N = m.Dim(1);
		
		Matrix<T> res (M, N, 2); 
		
#pragma omp parallel default (shared)
		{

#pragma omp for schedule (guided, 100) 
			for (size_t i = 0; i < N; i++) {
				for (size_t j = 0; j < M-1; j++)
					res(j,i,0) = m(j+1,i) - m(j,i);
				res (M-1,i,0) = T(0.0);
			}
			
#pragma omp for schedule (guided, 100) 
			for (size_t i = 0; i < M; i++) {
				for (size_t j = 0; j < N-1; j++)
					res(i,j,1) = m(i,j+1) - m(i,j);
				res (i,N-1,1) = T(0.0);
			}
			
		}
		
		return res;
		
	}	
	
	/**
	 * @brief    Backward transform
	 *
	 * @param  m To transform
	 * @return   Transform
	 */
	template <class T> static Matrix<T> 
	Adjoint    (Matrix<T>& m) {

		size_t M = m.Dim(0);
		size_t N = m.Dim(1);
		
		Matrix<cxfl> resx (M, N);
		Matrix<cxfl> resy (M, N);

		for (size_t i = 0; i < M; i++) {
			resy (0,i) = -m(0,i,0);
			for (size_t j = 1; j < N-1; j++)
				resy (j,i) = m(j-1,i,0) - m(j,i,0);
			resy (N-1,i) = m(N-2,i,0);
		}

		for (size_t i = 0; i < N; i++) {
			resx (i,0) = -m(i,0,1);
			for (size_t j = 1; j < M-1; j++)
				resx (i,j) = m(i,j-1,1) - m(i,j,1);
			resx (i,M-1) = m(i,M-2,1);
		}

		resx += resy;

		return resx;

	}
	

private:
	
	/**
	 * Static class
	 */
	TVOP()  {};

	/**
	 * Static class
	 */
	~TVOP() {};

};

#endif
