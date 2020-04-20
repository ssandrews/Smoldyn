//    Description: 
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  NCBS Bangalore

#include "Smoldyn.h"

Smoldyn::Smoldyn(): dim_(1)
{}

Smoldyn::~Smoldyn()
{}

SmoldynDefine& Smoldyn::getDefine()
{
    return define_;
}


size_t Smoldyn::getDim() const
{
    return dim_;
}

void Smoldyn::setDim(size_t dim)
{
    dim_ = dim;
}
