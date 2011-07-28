/*
 *  jrrs Copyright (C) 2007-2010 Kaveh Vahedipour
 *                               Forschungszentrum Jülich, Germany
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

#include "SpatialDomain.hpp"

const int    GRAD_RASTER = 10;
const double GAMMA       = 42576000;


using namespace RRStrategy;


/**
 * @brief           Phase correction from off-resonance
 *
 * @param  target   Target magnetisation
 * @param  result   Achieved result
 * @return          Phase corrected 
 */
Matrix<raw> 
PhaseCorrection              (Matrix<raw>* target, Matrix<raw>* result);

/**
 * @brief           Normalised root-means-squared error
 *
 * @param  target   Target magnetisation
 * @param  result   Achieved result
 * @return          NRMSE
 */
float       
NRMSE                         (const Matrix<raw>* target, const Matrix<raw>* result);

/**
 * @brief           RF limts
 *
 * @param  solution In:  Calculated solution
 * @param  pd       In:  Pulse durations
 * @param  nk       In:  # Pulses
 * @param  nc       In:  # Coils
 * @param  limits   Out: limits
 */
void        
RFLimits                      (const Matrix<raw>* solution, const int* pd, const int nk, const int nc, float* limits);


/**
 * @brief           STA magnetisation
 *
 * @param  k        k
 * @param  r        r
 * @param  b1       b1 map
 * @param  b0       b0 map
 * @param  nc       # of transmit channels
 * @param  nk       # of kspace positions
 * @param  ns       # of spatial positions
 * @param  gd       Gradient duration
 * @return          magnetisation
 */
Matrix<raw>
Magnetisation           (const Matrix<double>* ks, const Matrix<double>* r, const Matrix<raw>* b1, const Matrix<short>* b0, 
                         const int             nc, const int            nk, const int          ns, const int            gd);




SpatialDomain::SpatialDomain  () {}


SpatialDomain::~SpatialDomain () {
    
}


RRSModule::error_code
SpatialDomain::Init           ()  {

    RRSModule::error_code e = OK;

    Attribute ("Nk",      &m_nk); // # kt points

    m_pd = (int*) malloc (m_nk * sizeof(int));
    
    Attribute ("pd",      &m_pd[0]);   // Pulse durations    [10us]

    for (int i = 1; i < m_nk; i++)
        m_pd[i] = m_pd[0];

    Attribute ("gd",      &m_gd);      // Gradient durations [10us]
    Attribute ("Ns",      &m_ns);      // # of spatial sites
    Attribute ("Nc",      &m_nc);      // # of transmit channels
    Attribute ("maxiter", &m_maxiter); // Max # of iterations
    Attribute ("lambda",  &m_lambda);  // # Tikhonov parameter
    Attribute ("rflim",   &m_rflim);   // # RF limit (usualy in [0 1])
   
    return e;

}


RRSModule::error_code
SpatialDomain::Finalise       ()  {

    free (m_pd);

    return OK;

}


RRSModule::error_code
SpatialDomain::Process        () { 

    // On entry -------------------
    //
    // m_raw:     target pattern
    // m_kspace:  kt points
    // m_pixel:   b0 (Hz)
    // m_rhelper: b1+ maps
    // m_helper:  spatial positions
    // ----------------------------

    // On exit --------------------
    // 
    // m_raw:    Pulses
    // m_helper: Pulse durations
    // ----------------------------
    
    Matrix<raw> treg         = Matrix<raw>::id(m_nc * m_nk) * raw (m_lambda, 0);

    bool        pulse_amp_ok = false;
    float*      max_rf       = (float*) malloc (m_nk * sizeof(float));

    Matrix<raw> solution;
    Matrix<raw> tmp;
    Matrix<raw> final;    
    
    // Start clock ------------------------
    ticks cgstart = getticks();

    while (!pulse_amp_ok) {

        Matrix<raw> m    = Magnetisation (&m_kspace, &m_helper, &m_rhelper, &m_pixel, m_nc, m_nk, m_ns, m_gd);
        Matrix<raw> minv = m.tr();

        minv *= m;
        minv += treg;
        minv  = minv.Inv();    
        minv.dotc(m);
        
        // Valriable exchange method --------------

        float err = 1e6;
        std::vector<double> res;
        
        printf ("Variable exchange method ...\n");

        for (int j = 0; j < m_maxiter; j++) {
            
            solution = minv * m_raw;
            tmp      = m    * solution;

            res.push_back (NRMSE (&m_raw, &tmp));
            
            if (res.at(j) > err) break;
            if (res.at(j) < 2.5) break;
            
            err         = res.at(j) - 1.0e-3;
            
            final    = solution;
            m_raw    = PhaseCorrection (&m_raw, &tmp);
            
        }    
        
        printf ("... done. Checking pulse amplitudes ... \n");
        
        // Check max pulse amplitude -----------------
        RFLimits (&final, m_pd, m_nk, m_nc, max_rf);
        pulse_amp_ok = true;

        for (int i = 0; i < m_nk; i++) 
            if (max_rf[i] > m_rflim)
                pulse_amp_ok = false;
        
        // Update Pulse durations if necessary -------
        if (!pulse_amp_ok) {
            
            printf ("... done. Pulse amplitudes to high! Updating pulse durations ...\n");
            
            for(int i=0; i < m_nk;i++)
                m_pd[i] = 1 + (int) (max_rf[i] * m_pd[i] / m_rflim);

        }
        
        printf ("... done\n.");

    }

    free (max_rf);
    return RRSModule::OK;

}



float      
NRMSE                         (const Matrix<raw>* target, const Matrix<raw>* result) {

    float q = 0;
    float n = 0;

    for (int i = 0; i < target->Size(); i++)
        q += pow (abs(target->at(i)) - abs(result->at(i)), 2.0);

    q = sqrt(q)/target->norm().real();

    printf ("MRMSE: %.3f\n", q);

    return q * 100.0;

}



Matrix<raw> 
PhaseCorrection               (const Matrix<raw>* target, const Matrix<raw>* result) {

    Matrix<raw> tmp = *target;
    
#pragma omp parallel default (shared) 
    {
        
        int tid      = omp_get_thread_num();
        int chunk    = tmp.Size() / omp_get_num_threads();
        
#pragma omp for schedule (dynamic, chunk)
        
        for (int i = 0; i < tmp.Size(); i++) 
            if (abs(target->at(i)) > 0)    
                tmp[i] = abs (target->at(i)) * result->at(i) / abs(result->at(i));
            else                
                tmp[i] = raw (0.0,0.0);    

    }
    
    return tmp;
    
}


void 
RFLimits            (Matrix<raw> solution, float* pd, int nk, int nc, float* limits) {
    
    for (int i = 0; i < nk; i++) {
        
        limits[i] = 0.0;
        
        for (int j = 0; j < nc; j++)
            if (limits[i] < abs (solution [i+nk*j]) / pd[i]) 
                limits[i] = abs (solution [i+nk*j]) / pd[i];

    }
        
}


Matrix< std::complex<float> >
Magnetisation (const Matrix<double>* ks, const Matrix<double>* r, const Matrix< std::complex<float> >* b1, const Matrix<short>* b0, int nc, int nk, int ns, int gd) {
    
    Matrix< std::complex<float> > mxy;
    mxy.Dim(COL) = ns;
    mxy.Dim(LIN) = nc;
    mxy.Reset();
    
    std::complex<float> pgd = std::complex<float> (0, 2.0 * PI * GAMMA * GRAD_RASTER);            // 2* i * \pi * \gamma * \delta t
    
    for (int c = 0; c < nc; c++) 
        for (int k = 0; k < nk; k++) 
            for (int s = 0; s < ns; s++)
                mxy[c*nk + k*ns + s]  = 
                    pgd * b1->at(c*s + s) *                   // b1 (s,c) 
                    exp (raw(0, 2.0 * PI * gd * b0->at(s))) *  // delta b0:        exp (ib0t) 
                    exp (raw(0,(ks->at(0,k) * r->at(0,s) + 
                                ks->at(1,k) * r->at(1,s) + 
                                ks->at(2,k) * r->at(2,s))));   // kspace encoding: exp (ik(t)r)
    
    return mxy;

}


// the class factories
extern "C" DLLEXPORT ReconStrategy* create  ()                 {

    return new SpatialDomain;

}



extern "C" DLLEXPORT void           destroy (ReconStrategy* p) {

    delete p;

}