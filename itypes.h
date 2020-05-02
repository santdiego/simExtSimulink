/**********************************************************************************************
Author : Dr. Ing. Diego Daniel Santiago.
diego.daniel.santiago@gmail.com
dsantiago@inaut.unsj.edu.ar
Facultad de Ingeniería
Universidad Nacional de San Juan
San Juan - Argentina

INAUT - Instituto de Automática
http://www.inaut.unsj.edu.ar/
CONICET - Consejo Nacional de Investigaciones Científicas y Técnicas.
http://www.conicet.gov.ar/

Version 1.2
************************************************************************************************/
#ifndef ITYPES_H
#define ITYPES_H

#include <iostream>
#define M_PI       3.14159265358979323846   // pi
#define M_PI_2     1.57079632679489661923   // pi/2
#define M_PI_4     0.785398163397448309616  // pi/4

namespace ilib {
enum AccessMode:char {readWrite='0',read='r', write='w'};
inline std::istream& operator>>(std::istream& iss, AccessMode& acs){
    uint8_t aux=0;
    iss>>aux;
    acs=AccessMode(aux);
    return iss;
}

template <class T>
union union_data {
    unsigned char byte[sizeof (T)];
    T value;
};

template <class T>
T scale(T x, T in_min, T in_max, T out_min, T out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

}


#endif // ITYPES_H
