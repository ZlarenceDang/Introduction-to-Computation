#include "stdafx.h"
#include "random_convenience.h"

bool rand_zero_one()
{
	static random_device rand_dev;
	static ranlux48 rand_eng{ rand_dev() };
	static uniform_int_distribution<> zero_one_dist(0, 1);
	return zero_one_dist(rand_eng); 
}
float rand_zero2one()
{
	static random_device rand_dev;
	static ranlux48 rand_eng{ rand_dev() };
	static uniform_real_distribution<> zero2one_dist(0, 1);
	return zero2one_dist(rand_eng); 
}
float rand_normal()
{
	random_device rand_dev;
	ranlux48 rand_eng{ rand_dev() };
	normal_distribution<> normal_dist(0, 1);
	return normal_dist(rand_eng); 
}

float rand_clip_normal() { return tanh(rand_normal()); }
bool rand_chance(float pos) { return (pos > rand_zero2one()); }