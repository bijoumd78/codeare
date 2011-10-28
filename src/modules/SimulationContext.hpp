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

#ifndef __SIMULATION_CONTEXT_HPP__
#define __SIMULATION_CONTEXT_HPP__

#include "GPU.hpp"
#include "CPUSimulator.hpp"
#include "GPUSimulator.hpp"

namespace RRStrategy {

	
	/**
	 * @brief Context of a reconstruction method. Abstraction layer to algorithm backends. 
	 */
	class SimulationContext {
		
	public:

		/**
		 * @brief Constructor<br/>Check if we have GPU support or not and choose apporiate strategy
		 */
		SimulationContext () {
			m_strategy    = (SimulationStrategy*) (HaveGPU()) ? 
				(SimulationStrategy*) new GPUSimulator (): 
				(SimulationStrategy*) new CPUSimulator ();
		}


		/**
		 * @brief Destruct by deleting specific context
		 */
		virtual
		~SimulationContext () {
			delete m_strategy;
		}


		/**
		 * @brief       Piece-wise constant bloch simulation
		 *
		 * @see         SimulationStrategy::Simulate
		 */
		virtual void 
		Simulate  (const Matrix<cplx>&   txm, const Matrix<cplx>&   rxm, 
				   const Matrix<cplx>&    rf, const Matrix<double>&  gr, 
				   const Matrix<double>&   r, const Matrix<double>&  m0, 
				   const Matrix<double>& b0m, const double&          dt, 
				   const bool&           exc, const bool&             v, 
				   const size_t&          np, 
				         Matrix<cplx>&   res, Matrix<double>&         m) {

			m_strategy->Simulate (txm, rxm, rf, gr, r, m0, b0m, dt, exc, v, np, res, m);
			
		}
	


	private: 
		
		SimulationStrategy* m_strategy;
		
	};

}

#endif // __SIMULATION_CONTEXT_HPP__