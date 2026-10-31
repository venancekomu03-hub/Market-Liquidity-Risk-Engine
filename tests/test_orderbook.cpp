
#define ORDERBOOK_TESTING
#include "../main.cpp"

#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

int main() {
    OrderBook book;

    // Earlier seller should execute first.
    book.addOrder(1, "SELL", 10000, 50);
    book.addOrder(2, "SELL", 10000, 30);

    // Higher-priced seller should execute last.
    book.addOrder(3, "SELL", 10100, 40);

    // Buy 70 shares.
    book.addOrder(4, "BUY", 10100, 70);

    book.exportTrades("test_trades.csv");

    ifstream file("test_trades.csv");
    assert(file.is_open());

    string line;
    getline(file, line); // Header

    vector<vector<int>> trades;

    while (getline(file, line)) {
        stringstream ss(line);
        vector<int> values;
        string field;

        while (getline(ss, field, ',')) {
            values.push_back(stoi(field));
        }

        assert(values.size() == 4);
        trades.push_back(values);
    }

    assert(trades.size() == 2);

    // Expected CSV: buyId, sellId, price, quantity
    assert(trades[0][0] == 4);
    assert(trades[0][1] == 1);
    assert(trades[0][2] == 10000);
    assert(trades[0][3] == 50);

    assert(trades[1][0] == 4);
    assert(trades[1][1] == 2);
    assert(trades[1][2] == 10000);
    assert(trades[1][3] == 20);

    assert(book.cancelOrder(3));
    assert(!book.cancelOrder(3));

    cout << "\nALL ORDER BOOK TESTS PASSED!\n";
    return 0;
}
