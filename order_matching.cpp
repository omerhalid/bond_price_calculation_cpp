#include <string>
#include <chrono>
#include <queue>
#include <unordered_map>

struct Order {
    double price;
    int quantity;
    std::string orderId;
    bool isBuyOrder;
    std::chrono::time_point<std::chrono::system_clock> timestamp;

    // Constructor
    Order(double price_, int quantity_, std::string orderId_, bool isBuyOrder_)
        : price(price_), quantity(quantity_), orderId(orderId_), isBuyOrder(isBuyOrder_) ,timestamp(std::chrono::system_clock::now()) {}
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
    void removeFromHeap(std::priority_queue<Order, std::vector<Order>, Compare> &heap, std::string id)
    {

    }

    std::unordered_map<std::string, Order> orderMap;
    std::priority_queue<Order, std::vector<Order>, BuyOrderComparator> buyOrders;
    std::priority_queue<Order, std::vector<Order>, SellOrderComparator> sellOrders;

public:
    // i guess we need  to make this function lock free?
    void addOrder(double price, int quantity, std::string id, bool isBuyOrder)
    {
        if(orderMap.find(id) != orderMap.end())
            throw std::runtime_error("Order with the same ID already exists");

        Order newOrder(price, quantity, "bond1", isBuyOrder);
        orderMap[id] = newOrder;

        if(isBuyOrder) buyOrders.push(newOrder);
        else sellOrders.push(newOrder);

        // Call matchOrders to attempt to match the new order
        matchOrders();
    }

    void OrderBook::cancelOrder(std::string id)
    {
        if(orderMap.find(id) != orderMap.end())
        {
            if(orderMap[id].isBuyOrder) removeFromHeap(buyOrders, id);
            else removeFromHeap(sellOrders, id);

            orderMap.erase(id);
        }
    }

    void modifyOrders(double newPrice, int newQuantity, std::string id, bool isBuyOrder)
    {

    }

    void matchOrders()
    {

    }

};