#include "stdafx.h"
#include "evolution.h"
#include<iostream>

evolution::evolution(vector<gene> init_pool) :gene_pool(init_pool), fitnessl(init_pool.size(),0 ),gene_count(init_pool.size()){}

//evaluate fitness for all gene and modify
void evolution::eval_fitness()
{
	float max = 0, temp = 0, sum = 0, sum2 = 0, std = 0;

	max = fitnessl.front();
	//get fitness and standardize
	for (float temp : fitnessl)
	{
		//Here we track certain property we're interested in, e.g. max fitness
		if (max < temp) max = temp;
		sum += temp;
		sum2 += temp*temp;
	}
	sum /= gene_count;
	sum2 /= gene_count;
	std = sqrt((sum2-sum*sum)/(gene_count-1));

	//store those properties
	fit_info.push_back(track_info{max,sum,std});

	if (std == 0)
	{
		converged = 1;
		return;
	}

	cout << "result @ loop " << step << ":" << endl
		<< "max fitness: " << max << endl
		<< "mean fitness: " << sum << endl
		<< "fitness standard_div: " << std << endl << endl;
	//not that here we used 10.0f*exp(-.01f*i)+1+tanh to make weights positive and round related
	//coefs need to be tuned and function as well.
	for (float &i : fitnessl) i = 3.0f*exp(-.007f*step)+1.0f+tanh((i - sum) / std);
}
//selection in gene_pool according to fitness
void evolution::select()
{
	gene_pool = rand_choice(gene_pool, fitnessl, gene_count);
}
//cross-over
void evolution::crossover()
{
	gene temp1, temp2;
	for (int i = 1; i < gene_count; i += 2)
	{
		temp1.from_crossover(gene_pool.at(i - 1),gene_pool.at(i));
		temp2.from_crossover(gene_pool.at(i - 1),gene_pool.at(i));
		gene_pool.at(i - 1) = temp1;
		gene_pool.at(i) = temp2;
	}
}
//mutation
void evolution::mutate()
{
	//still need to be changed
	for (int i = 0; i < gene_count; i++) gene_pool.at(i).mutate(2.0f*exp(-.007f*step), .5f*exp(-.007f*step));
}

//all in all
void evolution::next_gen()
{
	eval_fitness();
	if (converged) return;
	select();
	crossover();
	mutate();
	step++;
}

ostream& operator << (ostream & ost, const track_info& track)
{
	ost << "max = " << track.max << endl;
	ost << "mean = " << track.mean << endl;
	ost << "standard deviation = " << track.std << endl;
	return ost;
}

void evolution::echo(ostream & ost, const int &ind) const
{
	ost << "evolution of round #" << ind << endl;
	for (int i = 0; i < gene_count; i++)
	{
		ost << endl;
		ost << "gene #" << i << endl;
		ost << "fitness = " << fitnessl[i] << endl;
		ost << gene_pool[i];
	}
}

