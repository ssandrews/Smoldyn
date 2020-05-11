/*
 * Geometry.h
 *
 * Copyright 2012 Martin Robinson
 *
 * This file is part of RD_3D.
 *
 * RD_3D is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RD_3D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RD_3D.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Created on: 19 Oct 2012
 *      Author: robinsonm
 */

#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include "Vector.h"
// extern "C"
//{
#include "random2.h"
//}
namespace Kairos {

class NullGeometry {
public:
    NullGeometry()
    {
    }
};

template <unsigned int DIM>
class AxisAlignedRectangle;

static const int dim_map[][2] = {{1, 2}, {0, 2}, {0, 1}};

template <unsigned int DIM>
class AxisAlignedPlane : public NullGeometry {
public:
    typedef AxisAlignedRectangle<DIM> SurfaceElementType;
    static const int                  dim = DIM;

    AxisAlignedPlane(const double coord, const int normal)
        : coord(coord), normal(normal)
    {
    }
    AxisAlignedPlane(const double coord) : coord(coord), normal(1.0)
    {
    }
    AxisAlignedPlane(const AxisAlignedPlane& arg)
        : coord(arg.coord), normal(arg.normal)
    {
    }

    bool is_between(const AxisAlignedPlane<DIM>& plane1,
        const AxisAlignedPlane<DIM>&             plane2) const
    {
        if(plane1.coord < plane2.coord) {
            return ((coord > plane1.coord) && (coord < plane2.coord));
        }
        else {
            return ((coord < plane1.coord) && (coord > plane2.coord));
        }
    }
    inline bool at_boundary(const Vect3d& r) const
    {
        return normal * (r[DIM] - coord) < 0;
    }
    inline bool at_boundary(const Vect3d& r, Vect3d& shortest) const
    {
        // shortest = Vect3d::Zero();
        shortest[dim_map[DIM][0]] = 0;
        shortest[dim_map[DIM][1]] = 0;
        shortest[DIM]             = coord - r[DIM];
        return normal * (-shortest[DIM]) < 0;
    }

    const Vect3d shortest_vector_to_boundary(const Vect3d& r) const
    {
        Vect3d shortest = Vect3d(0, 0, 0);
        shortest[DIM]   = coord - r[DIM];
        return shortest;
    }
    inline double distance_to_boundary(const Vect3d& r) const
    {
        return normal * (r[DIM] - coord);
    }
    const Vect3d operator-(const AxisAlignedPlane& arg) const
    {
        Vect3d diff = Vect3d(0, 0, 0);
        diff[DIM]   = coord - arg.coord;
        return diff;
    }
    void operator+=(const double move_by)
    {
        coord += normal * move_by;
    }
    void operator-=(const double move_by)
    {
        coord -= normal * move_by;
    }

    const double& get_coord() const
    {
        return coord;
    }

    const int& get_normal() const
    {
        return normal;
    }

    void swap_normal()
    {
        normal = -normal;
    }

protected:
    double coord;
    int    normal;
};

template <unsigned int DIM>
std::ostream& operator<<(std::ostream& out, const AxisAlignedPlane<DIM>& p)
{
    return out << "Plane aligned with dimension " << DIM;
}

std::ostream& operator<<(std::ostream& out, const NullGeometry& b);
std::ostream& operator<<(std::ostream& out, const AxisAlignedPlane<0>& p);
std::ostream& operator<<(std::ostream& out, const AxisAlignedPlane<1>& p);
std::ostream& operator<<(std::ostream& out, const AxisAlignedPlane<2>& p);

typedef AxisAlignedPlane<0> xplane;
typedef AxisAlignedPlane<1> yplane;
typedef AxisAlignedPlane<2> zplane;

template <unsigned int DIM>
class AxisAlignedRectangle : public AxisAlignedPlane<DIM> {
public:
    AxisAlignedRectangle(
        const Vect3d _low, const Vect3d _high, const int _normal)
        : AxisAlignedPlane<DIM>(_low[DIM], _normal),
          low(_low),
          high(_high),
          normal_vector(Vect3d(0, 0, 0))
    {
        high[DIM]          = low[DIM];
        normal_vector[DIM] = this->normal;
    }
    AxisAlignedRectangle(const AxisAlignedRectangle<DIM>& arg)
        : AxisAlignedPlane<DIM>(arg),
          low(arg.low),
          high(arg.high),
          normal_vector(arg.normal_vector)
    {
    }
    AxisAlignedRectangle<DIM>& operator=(const AxisAlignedRectangle<DIM>& arg)
    {
        AxisAlignedPlane<DIM>::operator=(arg);
        low                            = arg.low;
        high                           = arg.high;
        normal_vector                  = arg.normal_vector;
    }
    void get_random_point_and_normal(Vect3d& p, Vect3d& n)
    {
        p = get_random_point();
        n = normal_vector;
    }
    Vect3d get_random_point()
    {
        Vect3d ret;
        ret[DIM] = this->coord;
        ret[dim_map[DIM][0]] =
            unirandCCD(low[dim_map[DIM][0]], high[dim_map[DIM][0]]);
        ret[dim_map[DIM][1]] =
            unirandCCD(low[dim_map[DIM][1]], high[dim_map[DIM][1]]);
        return ret;
    }

    void get_random_point_and_normal_triangle(Vect3d& p, Vect3d& n)
    {
        p = get_random_point_triangle();
        n = normal_vector;
    }
    Vect3d get_random_point_triangle()
    {
        Vect3d ret;
        ret[DIM]        = this->coord;
        const double U1 = randCCD();
        if(U1 < 0.5) {
            ret[dim_map[DIM][0]] =
                low[dim_map[DIM][0]] +
                (high[dim_map[DIM][0]] - low[dim_map[DIM][0]]) * sqrt(0.5 * U1);
        }
        else {
            ret[dim_map[DIM][0]] =
                high[dim_map[DIM][0]] -
                (high[dim_map[DIM][0]] - low[dim_map[DIM][0]]) *
                    sqrt(0.5 * (1.0 - U1));
        }
        const double U2 = randCCD();
        if(U2 < 0.5) {
            ret[dim_map[DIM][1]] =
                low[dim_map[DIM][1]] +
                (high[dim_map[DIM][1]] - low[dim_map[DIM][1]]) * sqrt(0.5 * U2);
        }
        else {
            ret[dim_map[DIM][1]] =
                high[dim_map[DIM][1]] -
                (high[dim_map[DIM][1]] - low[dim_map[DIM][1]]) *
                    sqrt(0.5 * (1.0 - U2));
        }
        return ret;
    }

    const Vect3d& get_low() const
    {
        return low;
    }
    const Vect3d& get_high() const
    {
        return high;
    }

private:
    Vect3d low, high, normal_vector;
};

typedef AxisAlignedRectangle<0> xrect;
typedef AxisAlignedRectangle<1> yrect;
typedef AxisAlignedRectangle<2> zrect;

template <unsigned int DIM>
std::ostream& operator<<(std::ostream& out, const AxisAlignedRectangle<DIM>& p)
{
    return out << "Rectangle aligned with dimension " << DIM
               << ". Lower point in other dimensions is " << p.low
               << ". Upper point in other dimensions is " << p.high << ".";
}

class Rectangle {
public:
    Rectangle(const Vect3d& lower_corner, const Vect3d& upper_left_corner,
        const Vect3d& lower_right_corner) : low(lower_corner)
    {
        l      = upper_left_corner - lower_corner;
        r      = lower_right_corner - lower_corner;
        normal = cross(l, r);
        normal.normalize();
    }
    void get_random_point_and_normal(Vect3d& p, Vect3d& n)
    {
        p = get_random_point();
        n = normal;
    }
    Vect3d get_random_point()
    {
        return low + l * randCCD() + r * randCCD();
    }

    void get_random_point_and_normal_triangle(Vect3d& p, Vect3d& n)
    {
        p = get_random_point_triangle();
        n = normal;
    }
    Vect3d get_random_point_triangle()
    {
        const double U1 = randCCD();
        double       T1;
        if(U1 < 0.5) {
            T1 = sqrt(0.5 * U1);
        }
        else {
            T1 = 1.0 - sqrt(0.5 * (1.0 - U1));
        }
        const double U2 = randCCD();
        double       T2;
        if(U2 < 0.5) {
            T2 = sqrt(0.5 * U2);
        }
        else {
            T2 = 1.0 - sqrt(0.5 * (1.0 - U2));
        }
        return low + l * T1 + r * T2;
    }
    const Vect3d& get_low() const
    {
        return low;
    }
    const Vect3d& get_l() const
    {
        return l;
    }
    const Vect3d& get_r() const
    {
        return r;
    }
    const Vect3d& get_normal() const
    {
        return normal;
    }

private:
    Vect3d low;
    Vect3d l, r;
    Vect3d normal;
};

std::ostream& operator<<(std::ostream& out, const Rectangle& p);

}  // namespace Kairos

#endif /* GEOMETRY_H_ */
