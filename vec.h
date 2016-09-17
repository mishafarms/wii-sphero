#ifndef _VEC_H
#define _VEC_H

/**
 *	@struct vec2<T>
 *	@brief vectors of different types.
 */

template <class T>
class Vec2
{
public:
    T x;
    T y;

    Vec2()
    {
	x = (T) 0;
	y = (T) 0;
    }

    Vec2(T initX, T initY)
    {
	x = initX;
	y = initY;
    }

    ~Vec2()
    {
	// destructor does nothing yet 
    }

    Vec2 operator+ (const T & other)
    {
	T temp;

	temp.x = this->x + other.x;
	temp.y = this->y + other.y;
	return temp;
    }
};

/**
 *	@struct vec3<T>
 *	@brief vectors of different types.
 */

template <class T>
class Vec3
{
public:
    T x;
    T y;
    T z;

    Vec3()
    {
	x = (T) 0;
	y = (T) 0;
	z = (T) 0;
    }

    Vec3(T initX, T initY, T initZ)
    {
	x = initX;
	y = initY;
	z = initZ;
    }

    ~Vec3()
    {
	// destructor does nothing yet 
    }

    Vec3 operator+ (const T & other)
    {
	T temp;

	temp.x = this->x + other.x;
	temp.y = this->y + other.y;
	temp.z = this->z + other.z;
	return temp;
    }
};

/**
 *	@struct ang3<T>
 *	@brief 3 axis angles of different types.
 */

template <class T>
class Angle3
{
public:
    T pitch;
    T roll;
    T yaw;

    Angle3()
    {
	pitch = (T) 0;
	roll = (T) 0;
	yaw = (T) 0;
    }

    Angle3(T initPitch, T initRoll, T initYaw)
    {
	pitch = initPitch;
	roll = initRoll;
	yaw = initYaw;
    }

    ~Angle3()
    {
	// destructor does nothing yet 
    }
};
#endif /* _VEC_H */
