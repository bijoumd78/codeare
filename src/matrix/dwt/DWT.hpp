/*
 *  codeare Copyright (C) 2013 Daniel Joergens
 *                             Forschungszentrum Juelich, Germany
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
 *
 *  This code follow along the lines of WaveLab 850
 *
 */


# ifndef __DWT_HPP__

# define __DWT_HPP__

/**
 * OMP related makros
 */
# define NUM_THREADS_DWT 4
# define OMP_SCHEDULE guided


/**
 * @brief  Supported wavelet families
 */
enum wlfamily {

	ID = -1,                  /**< Identity transform*/
	WL_DAUBECHIES,
	WL_DAUBECHIES_CENTERED,
	WL_HAAR,
	WL_HAAR_CENTERED,
	WL_BSPLINE,
	WL_BSPLINE_CENTERED

};


# include "Matrix.hpp"
# include "Wavelet.hpp"



/**
 * @brief 2D Discrete wavelet transform for Matrix template (from GSL)
 */
template<class T>
class DWT {


    public:


        /**
         * @brief Construct 2D Wavelet transform with wavelet class and side length
         *
         * @param  sl      Side length
         */
        DWT (const size_t sl, const wlfamily wl_fam = WL_DAUBECHIES, const int wl_mem = 4,
        		const int wl_scale = 4, const int num_threads = NUM_THREADS_DWT, const int dim = 2)
            : _dim (dim),
              _sl1 (sl),
              _sl2 (sl),
              _fl (wl_mem),
              temp (container<T>(num_threads * 6 * sl)),
              _wl_scale (wl_scale),
              _fam(wl_fam),
              _num_threads (num_threads) {

            setupWlFilters <T> (wl_fam, wl_mem, _lpf_d, _lpf_r, _hpf_d, _hpf_r);

        }

        virtual
        ~DWT ()
        { }


        /**
         * @brief    Forward transform (no constructor calls)
         *
         * @param  m    Signal to decompose
         * @param  res  Resulting DWT
         */
        inline
        void
        Trafo        (const Matrix <T> & m, Matrix <T> & res)
        {

            assert (m.Size () <= _sl1 * _sl2
                    && m.Dim () == res.Dim ());

            // create vars from mex function
            int J = 0, nn;
            for (nn = 1; nn < m.Height (); nn *= 2 )
                J ++;
            if (nn  !=  m.Height ()){
                std::cout << "FWT2 requires dyadic length sides" << std::endl;
                assert (false);
            }

            // call dpwt2
            dpwt2 (m, _wl_scale, J, temp, res);

        }


        /**
         * @brief    Adjoint transform (no constructor calls)
         *
         * @param  m    DWT to transform
         * @param  res  Reconstructed signal
         */
        inline
        void
        Adjoint      (const Matrix <T> & m, Matrix <T> & res)
        {

            assert (m.Size () <= _sl1 * _sl2
                    && m.Dim () == res.Dim ());

            // create vars from mex function
            int J = 0, nn;
            for (nn = 1; nn < m.Height (); nn *= 2 )
                J ++;
            if (nn  !=  m.Height ()){
                std::cout << "IWT2 requires dyadic length sides" << std::endl;
                assert (false);
            }

            // call idpwt2
            idpwt2 (m, _wl_scale, J, temp, res);

        }


        /**
         * @brief    Forward transform
         *
         * @param  m To transform
         * @return   Transform
         */
        inline
        Matrix <T>
        operator*    (const Matrix <T> & m) {

            if (_fam == ID)
                return m;
            else
            {
                Matrix <T> res (m);
                Trafo (m, res);
                return res;
            }

        }


        /**
         * @brief    Adjoint transform
         *
         * @param  m To transform
         * @return   Transform
         */
        inline
        Matrix <T>
        operator->* (const Matrix <T> & m) {

            if (_fam == ID)
                return m;
            else
            {
                Matrix <T> res (m);
                Adjoint (m, res);
                return res;
            }

        }


    private:


        /**
         * type definitions
         */
        typedef typename TypeTraits <T> :: RT RT;


        /**
         * variable definitions
         */

        // dimension of DWT
        const int _dim;

        // low pass filters
        RT * _lpf_d;
        RT * _lpf_r;

        // high pass filters
        RT * _hpf_d;
        RT * _hpf_r;

        // maximum size of matrix
        const size_t _sl1; // side length in first dimension
        const size_t _sl2; // side length in second dimension

        // filter length
        const size_t _fl;

        // temporary memory
        container<T> temp;

        // wavelet scale (max. decomposition level)
        const int _wl_scale;

        const wlfamily _fam;

        const int _num_threads;


        /**
         * function definitions
         */


        /**
         *  COPIED FROM WAVELAB IMPLEMENTATION
         */

        void
        dpwt2		(const Matrix <T> & sig, const int ell, const int J,
             		 container <T> & temp, Matrix <T> & res)
        {

            T * wcplo, * wcphi, * templo, * temphi;

            // assign signal to result matrix
            res = sig;

            const int num_rows = sig.Height();
            const int num_cols = sig.Width();

#pragma omp parallel default (shared), private (wcplo, wcphi, temphi, templo) num_threads (_num_threads)
            {

                size_t stride;
                int side_length = sig.Height ();
                const int t_num = omp_get_thread_num ();

                // loop over levels of DWT
                for (int j = (J-1); j >= ell; --j)
                {

                    stride = side_length * t_num;

#pragma omp for schedule (OMP_SCHEDULE)
                    // loop over columns of image
                    for (int col=0; col < side_length; col++)
                    {

                        // access to lowpass part of DWT
                        wcplo = &res[col*num_rows];
                        // access to highpass part of DWT
                        wcphi = &res[col*num_rows + side_length/2];

                        // copy part of image to temp memory
                        copydouble (wcplo, &temp[0+stride], side_length);

                        // apply low pass filter on column and write to result matrix
                        downlo (&temp[0+stride], side_length, wcplo);
                        // apply high pass filter on column and wirte to result matrix
                        downhi (&temp[0+stride], side_length, wcphi);

                    } // loop over columns

                    // update stride
                    stride = 2 * side_length * t_num;

#pragma omp for schedule (OMP_SCHEDULE)
                    // loop over rows of image
                    for (int row=0; row < side_length; row++)
                    {

                        templo = &temp [      side_length + stride];
                        temphi = &temp [1.5 * side_length + stride];

                        // copy row-th row of imag to temp
                        unpackdouble (res.Memory(0), side_length, num_cols, row, &temp[0+stride]);

                        // apply low pass filter on row and write to temp mem
                        downlo (&temp[0+stride], side_length, templo);
                        // apply high pass filter on row and write to temp mem
                        downhi (&temp[0+stride], side_length, temphi);

                        // write temp lowpass result to result matrix
                        packdouble (templo, side_length/2, num_cols, row, &res[0]);
                        // write temp highpass result to result matrix
                        packdouble (temphi, side_length/2, num_cols, row, &res[side_length/2*num_rows]);

                    } // loop over rows of image

                    // reduce dimension for next level
                    side_length = side_length/2;

                } // loop over levels of DWT

            } // omp parallel

        }

        void
        unpackdouble	(const T * const x, const int n, const int nc, const int k, T * const y)
        {
            for (int i = 0; i < n; i++)
                y [i] = x [k + nc * i];
        }

        void
        packdouble		(const T * const x, const int n, const int nc, const int k, T * const y)
        {
            for (int i = 0; i < n; i++)
                y [k + nc * i] = x [i];
        }


        void
        copydouble		(const T * const src, T * const dest, const int n)
        {
        	memcpy (dest, src, n * sizeof (T));
        }


        void
        adddouble       (const T * const x, const T * const y, const int n, T * const z)
        {
            for (int i = 0; i < n; ++i)
                z[i] = x[i] + y[i];
        }


        /**
         * @brief               Perform highpass part of one level forward 1D DWT on signal with given side_length.
         *
         * @param  signal       Signal to be transformed.
         * @param  side_length  Side length of current DWT level.
         * @param  dwt_high     Resulting DWT.
         */
        void
        downhi			(const T * const signal, const int side_length, T * const dwt_high)
        {

            int j;
            T s;

            /* highpass version */

            // half of side_length
            const int n2 = side_length / 2;

            // half of filter length
            int mlo = _fl / 2 - 1;

            // adjust lower bound if to low
            if (2 * mlo + 1 - (_fl - 1) < 0)
                mlo++;

            // loop over pixels of dwt_high
            for (int i = mlo; i < n2; i++)
            {

                // result of convolution
                s = 0.;

                // perform convolution
                for (int h = 0; h < _fl; h++)
                    s += _hpf_d [h] * signal [2 * i + 1 - h];

                // assign result of convolution
                dwt_high [i] = s;

            } // loop over pixels of dwt_high

            // case: filter_length > side_length => only edge values
            if (mlo > n2)
                mlo = n2;


            /* fix up edge values */

            // loop over edge values
            for (int i = 0; i < mlo; i++)
            {

                // result of convolution
                s = 0.;

                // start signal index for convolution
                j = 2 * i + 1;

                // loop over filter elements
                for (int h = 0; h < _fl; h++)
                {

                    // adjust index if it exceeds side_length
                    if (j < 0)
                        j += side_length;

                    s += _hpf_d [h] * signal [j];

                    // update index
                    --j;

                }

                // assign result of convolution
                dwt_high [i] = s;

            } // loop over edge values

        }


        /**
         * @brief               Perform lowpass part of one level forward 1D DWT on signal with given side_length.
         *
         * @param  signal       Signal to be transformed.
         * @param  side_length  Side length of current DWT level.
         * @param  dwt_low      Resulting DWT.
         */
        void
        downlo  		        (const T * const signal, const int side_length, T * const dwt_low)
        {

            int j;
            T s;

            /*lowpass version */

            // half of side_length (length of dwt_low)
            const int n2 = side_length / 2;

            // half of filter_length
            const int mlo = _fl /2;

            // upper bound for "normal" convolution
            int mhi = n2 - mlo;

            // upper bound to high
            if (2 * mhi + (_fl - 1) >= side_length)
                --mhi;
            // upper bound to low
            if (mhi < 0)
                mhi = -1;

            // loop over pixels of dwt_low
            for (int i= 0; i <= mhi; i++)
            {

                // result of convolution
                s = 0.;
                // apply low pass filter (convolution)
                for (int h = 0; h < _fl; h++)
                    s += _lpf_d [h] * signal [2 * i + h];
                // assign result of convolution
                dwt_low [i] = s;

            } // loop over pixels of dwt_low


            /* fix up edge values */

            // loop over edge values (periodic boundary)
            for (int i = mhi + 1; i < n2; i++)
            {

                // result of convolution
                s = 0.;

                // start signal index for convolution
                j = 2 * i;

                // loop over filter elements
                for (int h = 0; h < _fl; h++){

                    // adjust index if it exceeds current side_length
                    if (j >= side_length)
                        j -= side_length;
                    s += _lpf_d [h] * signal [j];

                    // update index
                    j++;

                } // perform convolution

                // assign result of convolution
                dwt_low [i] = s;

            }

        }


        void
        idpwt2		(const Matrix <T> & wc, const int ell, const int J,
              		 container<T>& temp, Matrix <T> & img)
        {

            const int num_rows = wc.Height();
            const int num_cols = wc.Width();

            T * wcplo, * wcphi, * templo, * temphi, * temptop;

            // assign dwt to result image
            img = wc;

            // calculate start level for backwards DWT
            int side_length = 1;
            for (int k = 0; k < ell; k++)
                side_length *= 2;


#pragma omp parallel default (shared) firstprivate (side_length) private (wcplo, wcphi, temphi, templo, temptop) num_threads (_num_threads)
            {

                size_t stride;
                const int t_num = omp_get_thread_num ();

                // loop over levels of backwards DWT
                for (int j = ell; j < J; j++)
                {

                    stride = 6 * side_length * t_num;

#pragma omp for schedule (OMP_SCHEDULE)
                    // loop over rows of result image
                    for (int k = 0; k < 2 * side_length; k++)
                    {

                        templo  = &temp [2 * side_length + stride];
                        temphi  = &temp [3 * side_length + stride];
                        temptop = &temp [4 * side_length + stride];

                        // copy lowpass part of current row to temporary memory
                        unpackdouble(&img[0],side_length,num_cols,k,templo);
                        // copy highpass part of current row to temporary memory
                        unpackdouble(&img[side_length*num_rows],side_length,num_cols,k,temphi);

                        // perform lowpass reconstruction
                        uplo(templo, side_length,&temp[0+stride]);
                        // perform highpass reconstruction
                        uphi(temphi, side_length, temptop);

                        // fusion of reconstruction parts
                        adddouble(&temp[0+stride],temptop,side_length*2,&temp[0+stride]);

                        // write back reconstructed row
                        packdouble(&temp[0+stride],side_length*2,num_cols,k,&img[0]);

                    } // loop over rows of result image

                    // update stride
                    stride = 5 * side_length * t_num;

#pragma omp for schedule (OMP_SCHEDULE)
                    // loop  over cols of result image
                    for (int k = 0; k < 2 * side_length; k++)
                    {

                        templo = &temp [    side_length + stride];
                        temphi = &temp [3 * side_length + stride];
                        // assign address of current column's lowpass part
                        wcplo = &img[k*num_rows];
                        // assign address of current column's highpass part
                        wcphi = &img[k*num_rows + side_length];

                        // copy lowpass part to temporary memory
                        copydouble(wcplo,&temp[0+stride],side_length);

                        // perform lowpass reconstruction
                        uplo(wcplo, side_length, templo);
                        // perform highpass reconstruction
                        uphi(wcphi, side_length, temphi);

                        // combine reconstructed parts and write back to current column
                        adddouble(templo,temphi,side_length*2,wcplo);

                    } // loop over cols of result image


                    // update current row / column size
                    side_length *= 2;

                } // loop over levels of backwards DWT

            } // omp parallel

        }


        void
        uplo		(const T * const wc, const int side_length, T * const signal)
        {

            int j;
            T s, s_odd;

            /*lowpass version */

            /* away from edges */

            // upper bound for even filter indices
            const int meven = (_fl + 1) / 2;
            // upper bound for odd filter indices
            const int modd = _fl / 2;

            // loop over regular signal indices
            for (int i = meven; i < side_length; i++)
            {

                // init convolution results
                s = 0.;
                s_odd = 0.;

                // perform convolution for even and odd filter indices
                for (int h = 0; h < modd; h++)
                {

                    // even filter index
                    s += _lpf_r [2 * h] * wc [i - h];
                    // odd filter index
                    s_odd += _lpf_r [2 * h + 1] * wc [i - h];

                }
                // case of odd filter_length (-> more even indices: start with index 0)
                if (meven > modd)
                    s += _lpf_r [2 * meven] * wc [i - meven];

                // assign convolution results
                signal [2 * i] = s;
                signal [2 * i + 1] = s_odd;

            } // loop over regular signal indices


            /* fix up edge values */

            // upper bound for filter indices
            int mmax = meven;
            // possible correction if mmax greater than current side length
            if (mmax > side_length)
                mmax = side_length;

            // loop over edge values
            for (int i = 0; i < mmax; i++)
            {

                // init convolution results
                s = 0.;
                s_odd = 0.;
                // set start index of wavelet coefficients
                j = i;

                 // perform convolution
                for (int h = 0; h < modd; h++)
                {

                    // correct current wavelet coeff's index if needed
                    if (j < 0)
                        j += side_length;

                    // even part of convolution
                    s += _lpf_r [2 * h] * wc [j];
                    // odd part of convolution
                    s_odd += _lpf_r [2 * h + 1] * wc [j];

                    // update index
                    --j;

                } // perform convolution

                // case of odd filter_length
                if (meven > modd)
                    s += _lpf_r [2 * meven] * wc [j];

                // assign convolution results
                signal [2 * i] = s;
                signal [2 * i + 1] = s_odd;

            } // loop over edge values

        }


        void
        uphi		(const T * const wc, const int side_length, T * const signal)
        {

            int j;
            T s, s_odd;

            /*hipass version */

            // upper bound for even filter indices
            const int meven = (_fl + 1) / 2;
            // upper bound for odd filter indices
            const int modd = _fl / 2;

            /* away from edges */

            // loop over regular signal indices
            for (int i = 0; i < side_length - meven; i++)
            {

                // init convolution results
                s = 0.;
                s_odd = 0.;

                // perform convolution for even and odd filter indices
                for (int h = 0; h < modd; h++)
                {

                    // even filter index
                    s += _hpf_r [2 * h] * wc [i + h];
                    // odd filter index
                    s_odd += _hpf_r [2 * h + 1] * wc [i + h];

                } // perform convolution

                // case of odd filter_length
                if (meven > modd)
                    s += _hpf_r [2 * meven] * wc [i + meven];

                // assign convolution results
                signal [2 * i + 1] = s;
                signal [2 * i] = s_odd;

            } // loop over regular signal indices


            /* fix up edge values */

            // lower bound for indices of edge values
            int mmin = side_length - meven;
            // possible correction if mmin less than zero
            if (mmin < 0)
                mmin = 0;

            // loop over edge values
            for (int i = mmin; i < side_length; i++)
            {

                // init convolution results
                s = 0.;
                s_odd = 0.;
                // start index of wavelet coefficients
                j = i;

                // perform convolution for even and odd indices
                for (int h = 0; h < meven; h++)
                {

                    // correct current wavelet coeff's index if needed
                    if (j >= side_length)
                        j -= side_length;

                    // even filter index
                    s += _hpf_r [2 * h] * wc [j];
                    // odd filter index
                    s_odd += _hpf_r [2 * h + 1] * wc [j];

                    // update current wc index
                    j++;

                } // perform convolution

                // assign convolution results
                signal [2 * i + 1] = s;
                signal [2 * i] = s_odd;

            } // loop over edge values

        }



};


# endif // __DWT_HPP__
