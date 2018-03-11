#pragma once

//basics
//0 or 1
bool rand_zero_one();
//0 ~ 1
float rand_zero2one();
//normal distribution
float rand_normal();

//modified
//normal distribution from -1~1 after a tanh
float rand_clip_normal();
//random choice p->1,(1-p)->0
bool rand_chance(float);

//random choice of vector based on a chance vector
template<typename T>
T rand_choice(vector<T> &sel, vector<float> &num)
{
	if (sel.size() != num.size()) throw runtime_error("unequal length of objects and weights in rand_choice");

	float sum = 0;
	for (float i : num) sum += i;
	float rand_tmp = sum*rand_zero2one();
	sum = 0;

	for (int i = 0; i < num.size(); i++)
	{
		sum += num.at(i);
		if (sum > rand_tmp) return sel.at(i);
	}
}
template<typename T>
vector<T> rand_choice(vector<T> &sel, vector<float> &num, const unsigned int count)
{
	vector<T> temp{ count, sel.at(0) };
	for (int i = 0; i < count; i++) temp.at(i) = rand_choice(sel, num);
	return temp;
}