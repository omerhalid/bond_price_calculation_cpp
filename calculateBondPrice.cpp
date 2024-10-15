#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <future>
#include <mutex>
#include <chrono>
#include <random>
#include <algorithm>

// Bond struct remains the same
struct Bond {
    double faceValue;
    double couponRate;
    double yieldToMaturity;
    int yearsToMaturity;
};

// Shared portfolio data and mutex for thread safety
std::vector<Bond> portfolio;
std::mutex bondMutex;

// Function to calculate bond price
double calculateBondPrice(double faceValue, double couponRate, double yieldToMaturity, int yearsToMaturity)
{
    double bondPrice = 0.0;
    double couponPayment = couponRate * faceValue;
    double discountFactor = 1.0 + yieldToMaturity;

    for (int year = 1; year <= yearsToMaturity; ++year)
    {
        bondPrice += couponPayment / std::pow(discountFactor, year);
    }
    bondPrice += faceValue / std::pow(discountFactor, yearsToMaturity);

    return bondPrice;
}

// Simulate live updates by modifying bond data in batches
void simulateLiveDataUpdates()
{
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.03, 0.07);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulate real-time update frequency

        // Lock the portfolio for updating bond data
        {
            std::lock_guard<std::mutex> lock(bondMutex);

            // Update bond data in batches (e.g., batch size of 1000)
            size_t batchSize = 1000;
            for (size_t i = 0; i < portfolio.size(); i += batchSize)
            {
                size_t end = std::min(i + batchSize, portfolio.size());
                for (size_t j = i; j < end; ++j)
                {
                    portfolio[j].yieldToMaturity = distribution(generator); // Simulate live yield updates
                }
            }
        }

        std::cout << "Bond data updated in batches." << std::endl;
    }
}

// Price the bond portfolio using parallel processing with a thread pool
std::vector<double> priceBondPortfolioParallel(const std::vector<Bond>& portfolio, size_t threadCount)
{
    std::vector<std::future<std::vector<double>>> futures;
    futures.reserve(threadCount);

    size_t batchSize = portfolio.size() / threadCount;
    
    for (size_t i = 0; i < threadCount; ++i)
    {
        size_t start = i * batchSize;
        size_t end = (i == threadCount - 1) ? portfolio.size() : (i + 1) * batchSize;

        // Launch threads to process portfolio chunks in parallel
        futures.emplace_back(std::async(std::launch::async, [start, end, &portfolio]() {
            std::vector<double> prices;
            for (size_t j = start; j < end; ++j)
            {
                prices.emplace_back(calculateBondPrice(portfolio[j].faceValue, portfolio[j].couponRate, 
                                                       portfolio[j].yieldToMaturity, portfolio[j].yearsToMaturity));
            }
            return prices;
        }));
    }

    // Gather results from all threads
    std::vector<double> allPrices;
    allPrices.reserve(portfolio.size());

    for (auto& future : futures)
    {
        auto partialPrices = future.get();
        allPrices.insert(allPrices.end(), partialPrices.begin(), partialPrices.end());
    }

    return allPrices;
}

int main()
{
    // Initialize a large portfolio (simulate 10,000 bonds)
    portfolio = std::vector<Bond>(10000, {1000.0, 0.05, 0.04, 10});

    // Launch threads for live data updates and pricing calculations
    std::thread updateThread(simulateLiveDataUpdates);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3)); // Pricing frequency

        std::lock_guard<std::mutex> lock(bondMutex);
        
        // Calculate bond prices using parallel processing (4 threads for this example)
        std::vector<double> bondPrices = priceBondPortfolioParallel(portfolio, 4);

        std::cout << "Bond Prices (first 10):" << std::endl;
        for (size_t i = 0; i < std::min(size_t(10), bondPrices.size()); ++i)
        {
            std::cout << bondPrices[i] << std::endl;
        }
    }

    updateThread.join();

    return 0;
}
