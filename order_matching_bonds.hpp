#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <future>
#include <iostream>
#include <random>
#include <chrono>
#include <unordered_map>
#include <stdexcept>
#include <atomic>

// Bond prices vector (shared)
extern std::vector<double> bondPrices;

// Order structure
struct Order {
    double price;
    int quantity;
    std::string orderId;
    bool isBuyOrder;
    std::chrono::time_point<std::chrono::system_clock> timestamp;

    Order();
    Order(double price_, int quantity_, std::string orderId_, bool isBuyOrder_);

    bool operator > (const Order& other) const;
};

// OrderBook class
class OrderBook {
private:
    struct BuyOrderComparator {
        bool operator()(const Order& lhs, const Order& rhs);
    };

    struct SellOrderComparator {
        bool operator()(const Order& lhs, const Order& rhs);
    };

    template <typename Compare>
    void removeFromHeap(std::priority_queue<Order, std::vector<Order>, Compare> &heap, const std::string &id);

    std::unordered_map<std::string, Order> orderMap;
    std::priority_queue<Order, std::vector<Order>, BuyOrderComparator> buyOrders;
    std::priority_queue<Order, std::vector<Order>, SellOrderComparator> sellOrders;

    std::atomic_flag lock = ATOMIC_FLAG_INIT; // Simple spinlock

public:
    void addOrder(double price, int quantity, const std::string& id, bool isBuyOrder);
    void cancelOrder(const std::string& id);
    void modifyOrders(double newPrice, int newQuantity, const std::string& id, bool isBuyOrder);
    void matchOrders();
    void updateBondPrices(const std::vector<double>& newBondPrices); // New method to update bond prices
};