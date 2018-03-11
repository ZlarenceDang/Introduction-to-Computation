#pragma once
#include "stdafx.h"
#include "random_convenience.h"

typedef struct point
{
	float angle;
	float length;
};

//euclidean distance
float eu_dist(point,point);
//max length
float l_max(vector<point>);

//chance -> possibility to mutate
//size -> mutate how much? angle+=2pi*normaldistribution(size) length*=(1+normaldistribution(size))
void mutate_point(point&, float, float);
void mutate_radius(point&, float, float);
//generate a random point with random theta and r. but note that it's not a random point in a disk~
point rand_point();
vector<point> rand_point(const unsigned int &point_number);
//vector<point> rand_wheel(const unsigned int &point_number);

bool angle_smaller(point, point);
double cal_area(vector<point>&);
void normalize_area(vector<point>&);

//Plain Point
typedef struct ppoint
{
	float x;
	float y;
};

//convertion
ppoint polar2plain(point);
point plain2polar(ppoint);

//plus
ppoint operator+ (ppoint, ppoint);
bool operator < (const ppoint &, const ppoint &);