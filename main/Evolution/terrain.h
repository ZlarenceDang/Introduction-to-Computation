#pragma once

#include<cstdlib>
#include "geometry.h"

//psuedo - random number generator
typedef unsigned int PRNGINT;

constexpr PRNGINT PSEED = 20121221;
constexpr PRNGINT A = 1664525;
constexpr PRNGINT C = 1;

//perlin terrain
typedef struct perlin_para
{
	double x_step = 1.0;
	int default_ascending_step = 300;
	int default_wave_length = 20;
	int default_octave = 1;
	int octave_step = 1;
	double default_amplitude = 5.0;
	double amplitude_step = 5.0;
};

class PRNG
{
public:
	PRNG(PRNGINT SEED = PSEED) : now(SEED) {};
	double go()
	{
		now = now*A + C;
		return (double)now / 4294967296.0 -	0.5;
	}
private:
	PRNGINT now;
};

inline double Interpolate(const double &left, const double &right, const double &px);
vector<double> perlin_noise(int wave_length, double amplitude, int width,double last);
vector<double> combine_noise(int wave_length, double amplitude, int width, int octave_num,double last);
vector<ppoint> perlin_terrain(int length, const perlin_para&);

ostream& operator << (ostream& ost, const vector<ppoint>& terrain);

//flat terrain
vector<ppoint> flat_terrain(int length);

//exp terrain
const float exp_coef = 0.002f;
const float exp_ampl = 5.0f;
vector<ppoint> exp_terrain(int length);

//saw terrain
const float saw_period = 200.0f;
const float saw_length = 70.0f;
const float saw_height = 10.0f;
vector<ppoint> saw_terrain(int length);

//welcome
vector<ppoint> welcome_terrain(int length);