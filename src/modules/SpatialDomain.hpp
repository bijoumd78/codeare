/*
 *  codeare Copyright (C) 2007-2010 Kaveh Vahedipour
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

#ifndef __SPATIAL_DOMAIN_HPP__
#define __SPATIAL_DOMAIN_HPP__

#include "ReconStrategy.hpp"

/**
 * @brief Reconstruction startegies
 */
namespace RRStrategy {

	/**
	 * @brief Empty recon for test purposes
	 */
	class SpatialDomain : public ReconStrategy {
		
		
	public:
		
		/**
		 * @brief Default constructor
		 */
		SpatialDomain  () {};
		
		/**
		 * @brief Default destructor
		 */
		virtual 
		~SpatialDomain () {};
		
		
		/**
		 * @brief Do nothing 
		 */
		virtual RRSModule::error_code
		Process ();
		
		/**
		 * @brief Do nothing 
		 */
		virtual RRSModule::error_code
		Init () {
			return codeare::OK;
		}
		
		/**
		 * @brief Do nothing 
		 */
		virtual RRSModule::error_code
		Finalise () {

			return codeare::OK;

		}
		
	};

}
#endif /* __SPATIAL_DOMAIN_H__ */

