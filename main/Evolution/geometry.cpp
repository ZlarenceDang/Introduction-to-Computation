#include "stdafx.h"
#include "geometry.h"

float eu_dist(point a, point b)
{
	return sqrt(a.length*a.length + b.length*b.length - 2 * a.length*b.length*cos(a.angle - b.angle));
}

float l_max(vector<point> vec)
{
	float lmax = 0;
	for (point i : vec) if (lmax < i.length) lmax = i.length;
	return lmax;
}

 void mutate_point(point & pt, float chance, float size)
{
	if(rand_chance(chance)) pt.angle += size*2*pi*rand_normal();
	if(rand_chance(chance)) pt.length *= (1 + size*rand_clip_normal());
}

void mutate_radius(point & pt, float chance, float size)
{
	if (rand_chance(chance)) pt.length *= (1 + size*rand_clip_normal());
}

point rand_point()
{
	return point{ float(2 * pi*rand_zero2one()),float(2 * rand_zero2one()) };
}

vector<point> rand_wheel(const unsigned int &num)
{
	vector<point> res;
	res.reserve(num);
	float unit_ang = 2.0f*pi / (float)num;
	for (int i = 0; i < num; i++)
		res.push_back(point{ unit_ang*(float)i,float(2 * rand_zero2one()) });
	return res;
}

bool half_test(const vector<point>& r) {
	for (int i = 1; i < r.size(); i++) {
		if (fabs(r.at(i).angle - r.at(i - 1).angle) >= pi / 2.2
			|| fabs(r.at(i).angle - r.at(i - 1).angle) < pi / 60) return false;
	}
	if (fabs(r.at(0).angle - r.at(r.size() - 1).angle) >= pi / 2.2
		|| fabs(r.at(0).angle - r.at(r.size() - 1).angle) < pi / 60) return false;
	return true;
}

vector<point> rand_point(const unsigned int &n)
{
	vector<point> temp(n, point{ 0,0 });
	for (point& i : temp) i = rand_point();
	while (half_test(temp)) for (point& i : temp) i = rand_point();
	return temp;
}

bool angle_smaller(point pt1, point pt2) { return (pt1.angle < pt2.angle); }
double cal_area(vector<point>&ptl)
{
	for (point &i : ptl) i.angle = remainder(i.angle, 2 * (float)pi);
	//sort
	sort(ptl.begin(), ptl.end(), angle_smaller);
	//calculate area
	float area_tot = fabs(0.5*ptl.front().length*ptl.back().length*sin(ptl.front().angle + 2 * pi - ptl.back().angle));
	for (int i = 0; i < ptl.size() - 1; i++) area_tot += fabs(0.5*ptl.at(i).length*ptl.at(i + 1).length*sin(ptl.at(i + 1).angle - ptl.at(i).angle));
	return area_tot;
}
void normalize_area(vector<point>& ptl)
{
	float mult = 1/sqrt(cal_area(ptl));
	for (point &i : ptl) i.length *= mult;
}

ppoint polar2plain(point p) { return ppoint{ p.length*cos(p.angle),p.length*sin(p.angle) }; }
point plain2polar(ppoint p) { return point{ atan2f(p.y,p.x),sqrt(p.x*p.x + p.y*p.y) }; }

ppoint operator+(ppoint p1, ppoint p2) { return ppoint{ p1.x + p2.x,p1.y + p2.y }; }
bool operator < (const ppoint & p1, const ppoint & p2)
{
	return p1.x < p2.x;
}
