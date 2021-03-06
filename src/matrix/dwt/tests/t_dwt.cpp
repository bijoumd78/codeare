#include "Matrix.hpp"
#include "Algos.hpp"
#include "Creators.hpp"
#include "Params.hpp"
#include "DWT.hpp"


int main (int args, char** argv) {

    size_t sl = 256, sl3 = 128;
    
    Matrix<cxfl> A = phantom<cxfl>(sl), B, C;
    Matrix<cxfl> D = phantom3D<cxfl>(sl3), E, F;
    
    DWT<cxfl> wt (sl), wt3(sl3,sl3,sl3);
    B = wt * A;
    C = wt ->* B;
    E = wt3 * D;
    F = wt3 ->* E;
    
    return 0;
    
}
