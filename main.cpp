#include <iostream>
#include <cmath>
#include <random>

constexpr double spot = 219.0;
constexpr double rfr = 0.05;
constexpr double vol = 0.5;
constexpr unsigned numDays = 365 * 2;
constexpr double dt = 1.0 / numDays;
constexpr double strike = 250.0;
constexpr unsigned numPaths = 100000;
constexpr double df = 0.9;
unsigned seeds[numPaths];

double nextPrice(double currentPrice, double epsilon)
{
	double expTerm1 = (rfr - ((vol * vol) / 2.0)) * dt;
	double expTerm2 = vol * epsilon * sqrt(dt);
	return currentPrice * exp(expTerm1 + expTerm2);
}

void initSeeds()
{
	std::random_device rd;
	std::mt19937_64 mt(rd());
	for (unsigned i = 0; i < numDays; ++i)
		seeds[i] = mt();
}

double executePath(unsigned seed)
{
	std::mt19937_64 mt(seed);
	std::normal_distribution nd;

	std::vector<double> prices;
	prices.push_back(spot);

	double price = spot;
	for (int i = 0; i <= numDays; ++i)
	{
		price = nextPrice(price, nd(mt));
		//std::cout << "new price: " << price << "\n";
		prices.push_back(price);
	}

	//for (int i = 0; i < prices.size(); ++i)
	//	std::cout << "Price at t" << i << ": " << prices[i] << "\n";
	
	return price;
}

double getPayoff(double price)
{
	return std::max(price - strike, 0.0);
}

int main()
{
	initSeeds();

	std::vector<double> discPayoffs;
	discPayoffs.resize(numPaths);

	for (unsigned i = 0; i < numPaths; ++i)
	{
		double price = executePath(seeds[i]);
		double payoff = getPayoff(price);
		discPayoffs[i] = payoff * df;
	}

	double averagedPayoff = (1.0 / static_cast<double>(numPaths)) * std::accumulate(discPayoffs.begin(), discPayoffs.end(), 0.0);
	
	std::cout << "Option payoff = " << averagedPayoff << std::endl;	
}
