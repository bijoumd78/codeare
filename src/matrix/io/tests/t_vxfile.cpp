#include "VXFile.hpp"

using namespace codeare::matrix::io;

template<class T> inline static bool read (const std::string& fname, Matrix<T>& A) {
    VXFile vxf (fname, READ);
    vxf.Read();
    return true;
}

template<class T> inline static bool check () {
  
    Matrix<T> A;
    read("test.dat", A);
    std::cout << wspace << std::endl;
    return true;
  
}

int main (int args, char** argv) {
    if (args == 1) {
        if (check<cxfl>())
            return 0;
    } else {
        VXFile vxf (argv[1],READ);
        vxf.Read();
        std::cout << wspace << std::endl;
        return 0;
    }
    return 1;
}
