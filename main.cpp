
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <fstream>
#include <iomanip>
#include <random>
using namespace std;

struct Order {
    int id;
    string side;
    int price;       // Price in cents
    int quantity;
    int sequence;
};

struct Trade {
    int buyId;
    int sellId;
    int price;
    int quantity;
};

class OrderBook {
private:
    vector<Order> buyers;
    vector<Order> sellers;
    vector<Trade> trades;
    struct Snapshot {
    int event;
    int bestBid;
    int bestAsk;
    long long bidDepth;
    long long askDepth;
    double imbalance;
};

vector<Snapshot> history;

    set<int> usedIds;
    int nextSequence = 1;

    static bool buyPriority(const Order& a, const Order& b) {
        if (a.price != b.price)
            return a.price > b.price;

        return a.sequence < b.sequence;
    }

    static bool sellPriority(const Order& a, const Order& b) {
        if (a.price != b.price)
            return a.price < b.price;

        return a.sequence < b.sequence;
    }

    static void removeFilled(vector<Order>& orders) {
        orders.erase(
            remove_if(orders.begin(), orders.end(),
                [](const Order& o) {
                    return o.quantity == 0;
                }),
            orders.end()
        );
    }

public:

bool exportSlippage(const string& filename) const {
    ofstream file(filename);

    if (!file.is_open()) {
        return false;
    }

    file << "requested,filled,fill_rate,average_price,slippage_pct\n";

    vector<int> sizes = {
        100, 500, 1000, 2500, 5000, 10000, 20000
    };

    vector<Order> asks = sellers;

    sort(asks.begin(), asks.end(),
         [](const Order& a, const Order& b) {
             if (a.price != b.price)
                 return a.price < b.price;
             return a.sequence < b.sequence;
         });

    for (int requested : sizes) {
        int remaining = requested;
        int filled = 0;
        long long totalCost = 0;

        for (const Order& ask : asks) {
            int quantity = min(remaining, ask.quantity);

            totalCost += 1LL * quantity * ask.price;
            filled += quantity;
            remaining -= quantity;

            if (remaining == 0)
                break;
        }

        double fillRate = 100.0 * filled / requested;

        double averagePrice = 0;
        double slippage = 0;

        if (filled > 0 && !asks.empty()) {
            double averageCents = double(totalCost) / filled;

            averagePrice = averageCents / 100.0;

            slippage = 100.0 *
                (averageCents - asks.front().price) /
                asks.front().price;
        }

        file << requested << ","
             << filled << ","
             << fillRate << ","
             << averagePrice << ","
             << slippage << "\n";
    }

    return file.good();
}


void estimateBuySlippage(int requestedQuantity) const {
    if (requestedQuantity <= 0) {
        cout << "Invalid requested quantity.\n";
        return;
    }

    // Copy sellers so the real order book stays unchanged
    vector<Order> asks = sellers;

    // Cheapest price first; earliest order first at equal prices
    sort(asks.begin(), asks.end(),
         [](const Order& a, const Order& b) {
             if (a.price != b.price)
                 return a.price < b.price;
             return a.sequence < b.sequence;
         });

    if (asks.empty()) {
        cout << "No selling liquidity available.\n";
        return;
    }

    int remaining = requestedQuantity;
    int filled = 0;
    long long totalCostCents = 0;

    int bestAsk = asks.front().price;

    for (const Order& ask : asks) {
        if (remaining == 0)
            break;

        int quantity = min(remaining, ask.quantity);

        totalCostCents += 1LL * quantity * ask.price;
        filled += quantity;
        remaining -= quantity;
    }

    cout << "\n===== BUY EXECUTION ESTIMATE =====\n";
    cout << "Requested shares: " << requestedQuantity << "\n";
    cout << "Filled shares: " << filled << "\n";
    cout << "Unfilled shares: " << remaining << "\n";

    if (filled == 0)
        return;

    double averagePriceCents =
        double(totalCostCents) / filled;

    double slippagePct =
        (averagePriceCents - bestAsk) / bestAsk * 100.0;

    cout << "Best ask: $" << bestAsk / 100.0 << "\n";
    cout << "Average execution price: $"
         << averagePriceCents / 100.0 << "\n";
    cout << "Estimated cost: $"
         << totalCostCents / 100.0 << "\n";
    cout << "Estimated slippage: "
         << slippagePct << "%\n";

    if (remaining > 0)
        cout << "WARNING: Insufficient liquidity for full execution.\n";
}


void recordSnapshot(int event) {
    int bestBid = 0;
    int bestAsk = 0;

    long long bidDepth = 0;
    long long askDepth = 0;

    for (const Order& order : buyers) {
        bidDepth += order.quantity;
        bestBid = max(bestBid, order.price);
    }

    for (const Order& order : sellers) {
        askDepth += order.quantity;

        if (bestAsk == 0 || order.price < bestAsk) {
            bestAsk = order.price;
        }
    }

    double imbalance = 0.0;
    long long totalDepth = bidDepth + askDepth;

    if (totalDepth > 0) {
        imbalance = double(bidDepth - askDepth) / totalDepth;
    }

    history.push_back({
        event,
        bestBid,
        bestAsk,
        bidDepth,
        askDepth,
        imbalance
    });
}

bool exportHistory(const string& filename) const {
    ofstream file(filename);

    if (!file.is_open()) {
        return false;
    }

    file << "event,best_bid_cents,best_ask_cents,"
         << "bid_depth,ask_depth,imbalance\n";

    for (const Snapshot& snapshot : history) {
        file << snapshot.event << ","
             << snapshot.bestBid << ","
             << snapshot.bestAsk << ","
             << snapshot.bidDepth << ","
             << snapshot.askDepth << ","
             << snapshot.imbalance << "\n";
    }

    return file.good();
}

    void addOrder(int id, string side, int price, int quantity) {
        if (usedIds.count(id) || price <= 0 || quantity <= 0 ||
            (side != "BUY" && side != "SELL")) {
            cout << "REJECTED: Invalid or duplicate order\n";
            return;
        }

        usedIds.insert(id);
        Order incoming = {id, side, price, quantity, nextSequence++};

        if (side == "BUY") {
            sort(sellers.begin(), sellers.end(), sellPriority);

            for (Order& seller : sellers) {
                if (incoming.quantity == 0) break;
                if (seller.price > incoming.price) break;

                int traded = min(incoming.quantity, seller.quantity);

                trades.push_back({
                    incoming.id, seller.id, seller.price, traded
                });

                incoming.quantity -= traded;
                seller.quantity -= traded;

                cout << "TRADE: " << traded
                     << " shares at $" << seller.price / 100.0 << "\n";
            }

            removeFilled(sellers);

            if (incoming.quantity > 0)
                buyers.push_back(incoming);

        } else {
            sort(buyers.begin(), buyers.end(), buyPriority);

            for (Order& buyer : buyers) {
                if (incoming.quantity == 0) break;
                if (buyer.price < incoming.price) break;

                int traded = min(incoming.quantity, buyer.quantity);

                trades.push_back({
                    buyer.id, incoming.id, buyer.price, traded
                });

                incoming.quantity -= traded;
                buyer.quantity -= traded;

                cout << "TRADE: " << traded
                     << " shares at $" << buyer.price / 100.0 << "\n";
            }

            removeFilled(buyers);

            if (incoming.quantity > 0)
                sellers.push_back(incoming);
        }
    }

    bool cancelOrder(int id) {
        auto matchesId = [id](const Order& o) {
            return o.id == id;
        };

        auto b = remove_if(buyers.begin(), buyers.end(), matchesId);
        bool foundBuy = b != buyers.end();
        buyers.erase(b, buyers.end());

        auto s = remove_if(sellers.begin(), sellers.end(), matchesId);
        bool foundSell = s != sellers.end();
        sellers.erase(s, sellers.end());

        if (foundBuy || foundSell)
            cout << "CANCELLED: " << id << "\n";

        return foundBuy || foundSell;
    }

    void display() const {
        vector<Order> b = buyers;
        vector<Order> s = sellers;

        sort(b.begin(), b.end(), buyPriority);
        sort(s.begin(), s.end(), sellPriority);

        cout << "\n===== ORDER BOOK =====\n";

        cout << "BUYERS:\n";
        for (const Order& o : b)
            cout << "#" << o.id << " $"
                 << o.price / 100.0 << " x "
                 << o.quantity << "\n";

        cout << "SELLERS:\n";
        for (const Order& o : s)
            cout << "#" << o.id << " $"
                 << o.price / 100.0 << " x "
                 << o.quantity << "\n";
    }

    void printMetrics() const {
        long long bidDepth = 0;
        long long askDepth = 0;

        int bestBid = 0;
        int bestAsk = 0;

        for (const Order& o : buyers) {
            bidDepth += o.quantity;
            bestBid = max(bestBid, o.price);
        }

        for (const Order& o : sellers) {
            askDepth += o.quantity;

            if (bestAsk == 0 || o.price < bestAsk)
                bestAsk = o.price;
        }

        cout << "\n===== MARKET METRICS =====\n";
        cout << "Bid depth: " << bidDepth << "\n";
        cout << "Ask depth: " << askDepth << "\n";

        if (bestBid > 0 && bestAsk > 0) {
            double midpoint = (bestBid + bestAsk) / 2.0;
            double spread = 100.0 * (bestAsk - bestBid) / midpoint;

            cout << "Best bid: $" << bestBid / 100.0 << "\n";
            cout << "Best ask: $" << bestAsk / 100.0 << "\n";
            cout << "Spread: " << spread << "%\n";
        }

        if (bidDepth + askDepth > 0) {
            double imbalance = double(bidDepth - askDepth) /
                               (bidDepth + askDepth);

            cout << "Imbalance: " << imbalance << "\n";
        }
    }

    bool exportTrades(const string& filename) const {
        ofstream file(filename);

        if (!file.is_open())
            return false;

        file << "buy_id,sell_id,price_cents,quantity\n";

        for (const Trade& trade : trades) {
            file << trade.buyId << ","
                 << trade.sellId << ","
                 << trade.price << ","
                 << trade.quantity << "\n";
        }

        return file.good();
    }

    bool exportLiquidity(const string& filename) const {
        ofstream file(filename);

        if (!file.is_open())
            return false;

        long long bidDepth = 0, askDepth = 0;
        int bestBid = 0, bestAsk = 0;

        for (const Order& o : buyers) {
            bidDepth += o.quantity;
            bestBid = max(bestBid, o.price);
        }

        for (const Order& o : sellers) {
            askDepth += o.quantity;

            if (bestAsk == 0 || o.price < bestAsk)
                bestAsk = o.price;
        }

        double imbalance = 0.0;

        if (bidDepth + askDepth > 0)
            imbalance = double(bidDepth - askDepth) /
                        (bidDepth + askDepth);

        file << "best_bid_cents,best_ask_cents,"
             << "bid_depth,ask_depth,imbalance\n";

        file << bestBid << ","
             << bestAsk << ","
             << bidDepth << ","
             << askDepth << ","
             << imbalance << "\n";

        return file.good();
    }
};


void simulateMarket(OrderBook& book, int events) {
    mt19937 rng(42);

    uniform_int_distribution<int> sideDist(0, 1);
    uniform_int_distribution<int> priceDist(9800, 10400);
    uniform_int_distribution<int> quantityDist(10, 200);

    cout << "\n===== RUNNING MARKET SIMULATION =====\n";

    for (int i = 0; i < events; i++) {
        int id = 1000 + i;

        string side = sideDist(rng) == 0
            ? "BUY" : "SELL";

        int price = priceDist(rng);
        int quantity = quantityDist(rng);

        book.addOrder(id, side, price, quantity);
        book.recordSnapshot(i + 1);
    }

    cout << "\nSimulation completed: "
         << events << " order events processed.\n";
}

#ifndef ORDERBOOK_TESTING
int main() {
    cout << fixed << setprecision(2);

    OrderBook book;

    // Generate 1,000 simulated market orders
    simulateMarket(book, 1000);

    // Show final order book
    book.display();

    // Calculate final liquidity statistics
    book.printMetrics();
    book.estimateBuySlippage(100);
book.estimateBuySlippage(500);
book.estimateBuySlippage(1000);
book.estimateBuySlippage(2500);
book.estimateBuySlippage(5000);
book.estimateBuySlippage(10000);
book.estimateBuySlippage(20000);

    // Export executed trades
    if (book.exportTrades("data/trades.csv")) {
        cout << "\nTrades exported successfully.\n";
    }

    // Export final liquidity snapshot
    if (book.exportLiquidity("data/liquidity.csv")) {
        cout << "Liquidity exported successfully.\n";
    }

    if (book.exportHistory("data/market_history.csv")) {
    cout << "Market history exported successfully.\n";
}
    if (book.exportSlippage("data/slippage.csv")) {
    cout << "Slippage data exported successfully.\n";
}
    return 0;
}
#endif
