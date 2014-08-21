/*
 *  codeare Copyright (C) 2010-2011 Kaveh Vahedipour
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

#include "GRAPPA.hpp"
#include "Print.hpp"

using namespace RRStrategy;

codeare::error_code
GRAPPA::Init () {

	printf ("Intialising GRAPPA ...\n");

	Attribute ("nthreads",  &m_nthreads);
	Attribute ("lambda", &m_lambda);
	m_kernel_size = RHSList<size_t>("kernel_size");
	m_acceleration_factors = RHSList<size_t>("acceleration_factors");

	printf ("... done.\n\n");

	return codeare::OK;

}


codeare::error_code
GRAPPA::Prepare     () { 
	printf ("  Preparing %s ...\n", Name());
	p.Set("n_threads", m_nthreads);
	p.Set("lambda", m_lambda);
	p.Set("acceleration_factors", m_acceleration_factors);
	p.Set("ac_data", Get<cxfl>("ac_data"));       // Sensitivities
    m_ft = new CGRAPPA<double>(p);
	printf ("  ... done.\n\n");
	return codeare::OK;
}

// On entry ------------------------
//
// m_raw:     Measured k-spaces      O (Nc x NKx/RX x NKy/RY x NKz/RZ)
// m_rhelper: ACS scans if external  O (Nc x NACSX  x NACSY  x NACSZ)
// ---------------------------------

// On exit -------------------------
//
// m_raw:     Reconstructed k-spaces O (Nc x NKx    x NKy    x NKz)
// ---------------------------------

codeare::error_code
GRAPPA::Process     () { 

	ticks cgstart = getticks();

	printf ("  Processing GRAPPA ...\n");

	//Matrix<cxdb>& scan = Get<cxdb>("scan");


	printf ("... done.. WTime: %.4f seconds.\n\n",
			elapsed(getticks(), cgstart) / Toolbox::Instance()->ClockRate());

	return codeare::OK;

}



codeare::error_code
GRAPPA::Finalise () {

	return codeare::OK;

}



// the class factories
extern "C" DLLEXPORT ReconStrategy* create  ()                 {
    return new GRAPPA;
}

extern "C" DLLEXPORT void           destroy (ReconStrategy* p) {
    delete p;
}
