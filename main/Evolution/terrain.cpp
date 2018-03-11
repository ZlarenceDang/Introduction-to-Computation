#include "terrain.h"
#include<cmath>
PRNG PRAN;

inline double Interpolate(const double &left, const double &right, const double &px)
{
	double ft = px * pi, f = (1 - cos(ft)) * 0.5;
	return left * (1 - f) + right * f;
}

vector<double> perlin_noise(int wave_length, double amplitude, int width, double last)
{
	vector<double> res;
	double left = PRAN.go(), right = PRAN.go();
	for (int x = 0; x < width; x++)
		if (x%wave_length)
		{
			res.push_back(Interpolate(left, right, (double)(x%wave_length) / (double)wave_length) * amplitude);
		}
		else
		{
			left = right;
			right = PRAN.go();
			if (x == 0) left = last/(double)amplitude;
			res.push_back(left * (double)amplitude);
		}
	return res;
}

vector<double> combine_noise(int wave_length, double amplitude, int width, int octave_num, double last)
{
	vector<double> res = perlin_noise(wave_length, amplitude, width,last);
	for (int i = 1; i < octave_num; i++)
	{
		wave_length >>= 1;
		amplitude /= 2.0;
		vector<double> tmp = perlin_noise(wave_length, amplitude, width,0);
		for (int j = 0; j < width; j++)
			res[j] += tmp[j];
	}
	return res;
}
vector<ppoint> perlin_terrain(int length, const perlin_para &para)
{
	vector<ppoint> res;
	int current = 0;
	int wave_length = para.default_wave_length;
	double amplitude = para.default_amplitude;
	int octave_num = para.default_octave;
	double x_step = para.x_step;
	int step = para.default_ascending_step;
	while (current < length)
	{
		vector<double> tmp = combine_noise(wave_length, amplitude, step, octave_num, current ? res[current-1].y:0);
		//cout << "calling perlin:\n" << "wave length = " << wave_length << "\t amplitude =" << amplitude << endl;
		for (int i = 0; i < step; i++) res.push_back(ppoint{ (float)x_step * (current + i), (float)tmp[i] });
		current = res.size();
		if (octave_num<=6) octave_num += para.octave_step;
		if ((1 << octave_num) > wave_length)
		{
			wave_length <<= 1;
			amplitude *= 2.0f; 
		}
		//if (amplitude <= 30) 
			amplitude += para.amplitude_step;
	}
	while (res.size() > length) res.pop_back();
	return res;
}

ostream& operator << (ostream& ost, const vector<ppoint>& terrain)
{
	unsigned int sz = terrain.size();
	for (int i = 0; i < sz; i++)
	{
		ost << terrain[i].x << ' ' << terrain[i].y << endl;
	}
	return ost;
}

vector<ppoint> flat_terrain(int length)
{
	return vector<ppoint>{ppoint{ 0.0f,0.0f }, ppoint{ float(length),0.0f }};
}

vector<ppoint> exp_terrain(int length)
{
	vector<ppoint> pl{};
	pl.reserve(length / 10 + 1);
	for (int i = 0; i < length + 10; i += 10) pl.push_back(ppoint{ float(i),exp_ampl*(exp((exp_coef*i>10)?10:exp_coef*i) - 1) });
	return pl;
}

vector<ppoint> saw_terrain(int length)
{
	vector<ppoint> pl{};
	pl.reserve(length / saw_period * 3 + 1);
	for (int i = 0; i < length / saw_period + 1; i++)
	{
		pl.push_back(ppoint{ float(i*saw_period),0.0f });
		pl.push_back(ppoint{ float(i + 1)*saw_period - saw_length,0.0f });
		pl.push_back(ppoint{ float(i + 1)*saw_period,saw_height });
	}
	return pl;
}

vector<ppoint> welcome_terrain(int length)
{
	return perlin_terrain(length, perlin_para{ 1,500,30,3,0,10,0 });
}



