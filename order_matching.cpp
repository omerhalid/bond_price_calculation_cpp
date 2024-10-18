#include <string>
#include <chrono>
#include <queue>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <atomic>
#include <mutex>

struct Order {
    double price;
    int quantity;
    std::string orderId;
    bool isBuyOrder;
    std::chrono::time_point<std::chrono::system_clock> timestamp;

    // Default constructor
    Order() : price(0.0), quantity(0), orderId(""), isBuyOrder(true), timestamp(std::chrono::system_clock::now()) {}
    
    // Constructor
    Order(double price_, int quantity_, std::string orderId_, bool isBuyOrder_)
        : price(price_), quantity(quantity_), orderId(orderId_), isBuyOrder(isBuyOrder_), timestamp(std::chrono::system_clock::now()) {}

    bool operator > (const Order& other) const
    {
        if (price == other.price)
        {
            return timestamp > other.timestamp;
        }
        return price > other.price;
    }
};

class OrderBook
{
private:
    struct BuyOrderComparator
    {
        bool operator()(const Order& lhs, const Order& rhs)
        {
            return (lhs.price > rhs.price) || (lhs.price == rhs.price && lhs.timestamp > rhs.timestamp); 
        }
    };

    struct SellOrderComparator
    {
        bool operator()(const Order& lhs, const Order& rhs)
        {
            return (lhs.price < rhs.price) || (lhs.price == rhs.price && lhs.timestamp < rhs.timestamp); 
        }
    };

    template <typename Compare>
    void removeFromHeap(std::priority_queue<Order, std::vector<Order>, Compare> &heap, const std::string &id)
    {
        std::vector<Order> temp;
        temp.reserve(heap.size());
    
        while (!heap.empty())
        {
            if (id != heap.top().orderId)
            {
                temp.emplace_back(heap.top());
            }
            heap.pop();
        }
    
        for (const auto &order : temp)
        {
            heap.push(order);
        }
    }

    std::unordered_map<std::string, Order> orderMap;
    std::priority_queue<Order, std::vector<Order>, BuyOrderComparator> buyOrders;
    std::priority_queue<Order, std::vector<Order>, SellOrderComparator> sellOrders;

    std::atomic_flag lock = ATOMIC_FLAG_INIT; // Simple spinlock

public:
    void addOrder(double price, int quantity, std::string id, bool isBuyOrder)
    {
        while (lock.test_and_set(std::memory_order_acquire)); // Acquire lock

        if(orderMap.find(id) != orderMap.end())
        {
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

    void cancelOrder(std::string id)
    {
        while (lock.test_and_set(std::memory_order_acquire)); // Acquire lock

        if(orderMap.find(id) != orderMap.end())
        {
            if(orderMap[id].isBuyOrder) removeFromHeap(buyOrders, id);
            else removeFromHeap(sellOrders, id);

            orderMap.erase(id);
        }

        lock.clear(std::memory_order_release); // Release lock
    }

    void modifyOrders(double newPrice, int newQuantity, std::string id, bool isBuyOrder)
    {
        while (lock.test_and_set(std::memory_order_acquire)); // Acquire lock

        if(orderMap.find(id) != orderMap.end())
        {
            cancelOrder(id);
            addOrder(newPrice, newQuantity, id, isBuyOrder);
        }

        lock.clear(std::memory_order_release); // Release lock

        matchOrders();
    }

    void matchOrders()
    {
        while (lock.test_and_set(std::memory_order_acquire)); // Acquire lock

        while(!buyOrders.empty() && !sellOrders.empty())
        {
            Order sellOrder = sellOrders.top();
            Order buyOrder = buyOrders.top();

            if(buyOrder > sellOrder) // overloaded > operator for Order type
            {
                auto matchedQuantity {std::min(buyOrder.quantity, sellOrder.quantity)};

                buyOrder.quantity -= matchedQuantity;
                sellOrder.quantity -= matchedQuantity;

                buyOrders.pop();
                sellOrders.pop();

                if(buyOrder.quantity > 0)
                {
                    buyOrders.push(buyOrder);
                }
                if(sellOrder.quantity > 0)
                {
                    sellOrders.push(sellOrder);
                }

                std::cout << "Matched " << matchedQuantity << " units at price " << sellOrder.price << std::endl;
            }
            else
            {
                break; // no match found, exit the loop
            }
        }

        lock.clear(std::memory_order_release); // Release lock
    }
};

int main() {
    OrderBook orderBook;

    try {
        orderBook.addOrder(100.5, 10, "order123", true);  // Buy order
        orderBook.addOrder(101.0, 5, "order124", false);  // Sell order
        orderBook.addOrder(99.5, 20, "order125", true);   // Buy order
        orderBook.addOrder(100.0, 15, "order126", false); // Sell order

        // Attempt to add an order with a duplicate ID
        orderBook.addOrder(102.0, 15, "order123", false); // This will throw an exception
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}