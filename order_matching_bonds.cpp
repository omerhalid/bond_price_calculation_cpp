#include "order_matching_bonds.hpp"

// Define bond prices vector
std::vector<double> bondPrices;

// Order structure implementation
Order::Order() : price(0.0), quantity(0), orderId(""), isBuyOrder(true), timestamp(std::chrono::system_clock::now()) {}

Order::Order(double price_, int quantity_, std::string orderId_, bool isBuyOrder_)
    : price(price_), quantity(quantity_), orderId(orderId_), isBuyOrder(isBuyOrder_), timestamp(std::chrono::system_clock::now()) {}

bool Order::operator > (const Order& other) const {
    if (price == other.price) {
        return timestamp > other.timestamp;
    }
    return price > other.price;
}

// OrderBook class implementation
void OrderBook::addOrder(double price, int quantity, const std::string& id, bool isBuyOrder) {
    while (lock.test_and_set(std::memory_order_acquire)); // Acquire lock

    if(orderMap.find(id) != orderMap.end()) {
        lock.clear(std::memory_order_release); // Release lock
        throw std::runtime_error("Order with the same ID already exists");
    }

    Order newOrder(price, quantity, id, isBuyOrder);
    orderMap[id] = newOrder;

    if(isBuyOrder) buyOrders.push(newOrder);
    else sellOrders.push(newOrder);

    lock.clear(std::memory_order_release); // Release lock

    // Call matchOrders to attempt to match the new order
    matchOrders();
}

void OrderBook::cancelOrder(const std::string& id) {
    while (lock.test_and_set(std::memory_order_acquire)); // Acquire lock

    if(orderMap.find(id) != orderMap.end()) {
        if(orderMap[id].isBuyOrder) removeFromHeap(buyOrders, id);
        else removeFromHeap(sellOrders, id);

        orderMap.erase(id);
    }

    lock.clear(std::memory_order_release); // Release lock
}

void OrderBook::modifyOrders(double newPrice, int newQuantity, const std::string& id, bool isBuyOrder) {
    while (lock.test_and_set(std::memory_order_acquire)); // Acquire lock

    if(orderMap.find(id) != orderMap.end()) {
        cancelOrder(id);
        addOrder(newPrice, newQuantity, id, isBuyOrder);
    }

    lock.clear(std::memory_order_release); // Release lock

    matchOrders();
}

void OrderBook::matchOrders() {
    while (lock.test_and_set(std::memory_order_acquire)); // Acquire lock

    while(!buyOrders.empty() && !sellOrders.empty()) {
        Order sellOrder = sellOrders.top();
        Order buyOrder = buyOrders.top();

        // Use bondPrices to determine if orders should be matched
        // For simplicity, let's assume we use the first bond price for matching
        double currentBondPrice = bondPrices[0];

        if(buyOrder.price >= sellOrder.price && buyOrder.price >= currentBondPrice) {
            int matchedQuantity = std::min(buyOrder.quantity, sellOrder.quantity);
            buyOrder.quantity -= matchedQuantity;
            sellOrder.quantity -= matchedQuantity;

            buyOrders.pop();
            sellOrders.pop();

            if(buyOrder.quantity > 0) {
                buyOrders.push(buyOrder);
            }
            if(sellOrder.quantity > 0) {
                sellOrders.push(sellOrder);
            }

            std::cout << "Matched " << matchedQuantity << " units at price " << sellOrder.price << std::endl;
        } else {
            break;
        }
    }

    lock.clear(std::memory_order_release); // Release lock
}

void OrderBook::updateBondPrices(const std::vector<double>& newBondPrices) {
    while (lock.test_and_set(std::memory_order_acquire)); // Acquire lock
    bondPrices = newBondPrices;
    lock.clear(std::memory_order_release); // Release lock
}

bool OrderBook::BuyOrderComparator::operator()(const Order& lhs, const Order& rhs) {
    return (lhs.price > rhs.price) || (lhs.price == rhs.price && lhs.timestamp > rhs.timestamp);
}

bool OrderBook::SellOrderComparator::operator()(const Order& lhs, const Order& rhs) {
    return (lhs.price < rhs.price) || (lhs.price == rhs.price && lhs.timestamp < rhs.timestamp);
}

template <typename Compare>
void OrderBook::removeFromHeap(std::priority_queue<Order, std::vector<Order>, Compare> &heap, const std::string &id) {
    std::vector<Order> temp;
    temp.reserve(heap.size());

    while (!heap.empty()) {
        if (id != heap.top().orderId) {
            temp.emplace_back(heap.top());
        }
        heap.pop();
    }

    for (const auto &order : temp) {
        heap.push(order);
    }
}