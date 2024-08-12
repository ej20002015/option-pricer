#include <iostream>
#include <cmath>
#include <random>
#include <chrono>
#include <ratio>
#include <future>

constexpr double spot = 219.0;
constexpr double rfr = 0.05;
constexpr double vol = 0.5;
constexpr unsigned numDays = 365 * 5;
constexpr double dt = 1.0 / numDays;
constexpr double strike = 250.0;
constexpr unsigned numPaths = 65536;
constexpr double df = 0.9;
unsigned seeds[numPaths];

class Timer
{
public:
	Timer(const std::string& name) : m_tmStart(std::chrono::high_resolution_clock::now()), m_name(name)
	{
		std::cout << "Timer " << m_name << " started" << std::endl;
	}

	~Timer()
	{
		const auto tmEnd = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<double, std::milli> tmElapsed = tmEnd - m_tmStart;
		std::cout << "Timer " << m_name << " finished: " << tmElapsed << std::endl;
	}

private:
	std::string m_name;
	std::chrono::high_resolution_clock::time_point m_tmStart;
};

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
	for (unsigned i = 0; i < numPaths; ++i)
		seeds[i] = mt();
}

double getPrice(unsigned seed)
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
	
	return prices.back();
}

double getPayoff(double price)
{
	return std::max(price - strike, 0.0);
}

double executePath(unsigned seed)
{
	double price = getPrice(seed);
	double payoff = getPayoff(price);
	return payoff * df;
}

void executeManyPaths(const unsigned start, const unsigned end, std::vector<double>& discPayoffs)
{
	for (unsigned i = start; i < end; ++i)
		discPayoffs[i] = executePath(seeds[i]);
}	

int main()
{
	initSeeds();

	{
		Timer tm("Serial");

		std::vector<double> discPayoffs(numPaths);
	
		for (unsigned i = 0; i < numPaths; ++i)
			discPayoffs[i] = executePath(seeds[i]);

		double averagedPayoff = (1.0 / static_cast<double>(numPaths)) * std::accumulate(discPayoffs.begin(), discPayoffs.end(), 0.0);
		
		std::cout << "Option payoff = " << averagedPayoff << std::endl;	

	}

	{
		Timer tm("Parallel");
		
		std::vector<double> discPayoffs(numPaths);
		

		static constexpr unsigned NUM_THREADS = 16;
		static_assert(numPaths % NUM_THREADS == 0);
		static constexpr unsigned PATHS_PER_THREAD = numPaths / NUM_THREADS;
		std::vector<std::future<void>> fts(NUM_THREADS);
		
		for(unsigned i = 0; i < NUM_THREADS; ++i)
			fts[i] = std::async(executeManyPaths, i*PATHS_PER_THREAD, (i + 1) * PATHS_PER_THREAD, std::ref(discPayoffs));

		for (unsigned i = 0; i < NUM_THREADS; ++i)
			fts[i].get();
		
		//std::vector<std::future<double>> fts(numPaths);
		//for (unsigned i = 0; i < numPaths; ++i)
			//fts[i] = std::async(executePath, seeds[i]);

		//for (unsigned i = 0; i < numPaths; ++i)
			//discPayoffs[i] = fts[i].get();

		double averagedPayoff = (1.0 / static_cast<double>(numPaths)) * std::accumulate(discPayoffs.begin(), discPayoffs.end(), 0.0);
		
		std::cout << "Option payoff = " << averagedPayoff << std::endl;	

	}
}
