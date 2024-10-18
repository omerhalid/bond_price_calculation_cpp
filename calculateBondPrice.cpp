#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <future>
#include <mutex>
#include <chrono>
#include <random>
#include <algorithm>

// Separate arrays for bond attributes
std::vector<double> faceValues;
std::vector<double> couponRates;
std::vector<double> yieldsToMaturity;
std::vector<int> yearsToMaturity;

// Shared portfolio data and mutex for thread safety
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
            for (size_t i = 0; i < yieldsToMaturity.size(); i += batchSize)
            {
                size_t end = std::min(i + batchSize, yieldsToMaturity.size());
                for (size_t j = i; j < end; ++j)
                {
                    yieldsToMaturity[j] = distribution(generator); // Simulate live yield updates
                }
            }
        }

        std::cout << "Bond data updated in batches." << std::endl;
    }
}

// Price the bond portfolio using parallel processing with a thread pool
std::vector<double> priceBondPortfolioParallel(size_t threadCount)
{
    std::vector<std::future<std::vector<double>>> futures;
    futures.reserve(threadCount);

    size_t batchSize = faceValues.size() / threadCount;
    
    for (size_t i = 0; i < threadCount; ++i)
    {
        size_t start = i * batchSize;
        size_t end = (i == threadCount - 1) ? faceValues.size() : (i + 1) * batchSize;

        // Launch threads to process portfolio chunks in parallel
        futures.emplace_back(std::async(std::launch::async, [start, end]() {
            std::vector<double> prices;
            for (size_t j = start; j < end; ++j)
            {
                prices.emplace_back(calculateBondPrice(faceValues[j], couponRates[j], 
                                                       yieldsToMaturity[j], yearsToMaturity[j]));
            }
            return prices;
        }));
    }

    // Gather results from all threads
    std::vector<double> allPrices;
    allPrices.reserve(faceValues.size());

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
    size_t portfolioSize = 10000;
    faceValues.resize(portfolioSize, 1000.0);
    couponRates.resize(portfolioSize, 0.05);
    yieldsToMaturity.resize(portfolioSize, 0.04);
    yearsToMaturity.resize(portfolioSize, 10);

    // Launch threads for live data updates and pricing calculations
    std::thread updateThread(simulateLiveDataUpdates);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3)); // Pricing frequency

        std::lock_guard<std::mutex> lock(bondMutex);
        
        // Calculate bond prices using parallel processing (4 threads for this example)
        std::vector<double> bondPrices = priceBondPortfolioParallel(4);

        std::cout << "Bond Prices (first 10):" << std::endl;
        for (size_t i = 0; i < std::min(size_t(10), bondPrices.size()); ++i)
        {
            std::cout << bondPrices[i] << std::endl;
        }
    }

    updateThread.join();

    return 0;
}