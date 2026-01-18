// ============================================================================
// VISUALIZER.CPP - Order Book Visualization implementation
// ============================================================================

#include "Visualizer.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace orderbook {

// ============================================================================
// CONSTRUCTOR
// ============================================================================

Visualizer::Visualizer(const OrderBook& orderBook, const std::string& symbol)
    : m_orderBook(orderBook)
    , m_symbol(symbol)
{
#ifdef _WIN32
    // Enable ANSI colors on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

// ============================================================================
// DISPLAY METHODS
// ============================================================================

void Visualizer::render(size_t levels) const {
    clearScreen();
    printPriceTicker();      // Show prominent price display
    printPriceChart();       // Show price history chart
    printHeader();
    printOrderBook(levels);
    printFooter();
    printRecentTrades(5);
}

void Visualizer::clearScreen() const {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[H";
#endif
}

void Visualizer::printHeader() const {
    std::cout << "\n";
    printLine('=', 62);
    
    // Display sentiment if controller is set
    if (m_sentimentController) {
        Sentiment s = m_sentimentController->getSentiment();
        std::string sentimentColor = MarketSentimentController::getSentimentColor(s);
        std::string sentimentName = m_sentimentController->getCurrentSentimentName();
        std::cout << "  MARKET: " << sentimentColor << sentimentName << "\033[0m";
        std::cout << "     |     SYMBOL: " << m_symbol << "\n";
        std::cout << "  [Press 1-6 to change: 1=Bullish 2=Bearish 3=Volatile 4=Calm 5=Choppy 6=Neutral]\n";
    } else {
        std::cout << "                    ORDER BOOK - " << m_symbol << "\n";
    }
    printLine('=', 62);
}

void Visualizer::printOrderBook(size_t levels) const {
    auto bids = m_orderBook.getTopBids(levels);
    auto asks = m_orderBook.getTopAsks(levels);
    
    // Make both vectors the same size by padding with empty entries
    while (bids.size() < levels) {
        bids.emplace_back(0.0, 0);
    }
    while (asks.size() < levels) {
        asks.emplace_back(0.0, 0);
    }
    
    // Print column headers
    std::cout << "\n";
    std::cout << colorize("        BIDS (Buy)        ", "green");
    std::cout << " | ";
    std::cout << colorize("        ASKS (Sell)       ", "red");
    std::cout << "\n";
    printLine('-', 62);
    
    // Print price levels
    for (size_t i = 0; i < levels; ++i) {
        // Bid side (show in reverse order so highest is at top)
        size_t bidIdx = i;
        if (bidIdx < bids.size() && bids[bidIdx].second > 0) {
            std::cout << colorize(formatQuantity(bids[bidIdx].second), "green");
            std::cout << " x ";
            std::cout << colorize(formatPrice(bids[bidIdx].first), "green");
        } else {
            std::cout << std::setw(24) << " ";
        }
        
        // Separator
        if (i == 0) {
            std::cout << " <=> ";  // Best bid/ask indicator
        } else {
            std::cout << "  |  ";
        }
        
        // Ask side
        if (i < asks.size() && asks[i].second > 0) {
            std::cout << colorize(formatPrice(asks[i].first), "red");
            std::cout << " x ";
            std::cout << colorize(formatQuantity(asks[i].second), "red");
        }
        
        std::cout << "\n";
    }
    
    printLine('-', 62);
}

void Visualizer::printFooter() const {
    auto bestBid = m_orderBook.getBestBid();
    auto bestAsk = m_orderBook.getBestAsk();
    auto spread = m_orderBook.getSpread();
    
    std::cout << "\n";
    std::cout << "  Best Bid: ";
    if (bestBid) {
        std::cout << colorize("$" + formatPrice(*bestBid), "green");
    } else {
        std::cout << "N/A";
    }
    
    std::cout << "  |  Best Ask: ";
    if (bestAsk) {
        std::cout << colorize("$" + formatPrice(*bestAsk), "red");
    } else {
        std::cout << "N/A";
    }
    
    std::cout << "  |  Spread: ";
    if (spread) {
        std::cout << "$" << std::fixed << std::setprecision(2) << *spread;
    } else {
        std::cout << "N/A";
    }
    
    std::cout << "\n";
    
    // Statistics
    std::cout << "  Bid Levels: " << m_orderBook.getBidLevelCount();
    std::cout << "  |  Ask Levels: " << m_orderBook.getAskLevelCount();
    std::cout << "  |  Total Orders: " << m_orderBook.getTotalOrderCount();
    std::cout << "\n";
    
    printLine('=', 62);
}

// ============================================================================
// PRICE TICKER - Session stats display
// ============================================================================

void Visualizer::printPriceTicker() const {
    // Use LAST TRADE PRICE as the current price (this is how real markets work!)
    // If no trades yet, fall back to order book mid
    Price currentPrice = m_lastPrice;
    
    if (currentPrice <= 0) {
        // No trades yet - use order book mid as fallback
        auto bestBid = m_orderBook.getBestBid();
        auto bestAsk = m_orderBook.getBestAsk();
        
        if (!bestBid && !bestAsk) {
            return;  // No price to show
        }
        
        if (bestBid && bestAsk) {
            currentPrice = (*bestBid + *bestAsk) / 2.0;
        } else if (bestBid) {
            currentPrice = *bestBid;
        } else {
            currentPrice = *bestAsk;
        }
        
        // Initialize price tracking from order book
        updatePriceTracking(currentPrice);
    }
    
    // NOTE: Price tracking is now updated in addTrade() from actual trade prices!
    
    // Just show a compact session stats line - main price is in chart
    std::ostringstream highStr, lowStr, openStr;
    highStr << std::fixed << std::setprecision(2) << m_sessionHighPrice;
    lowStr << std::fixed << std::setprecision(2) << m_sessionLowPrice;
    openStr << std::fixed << std::setprecision(2) << m_sessionOpenPrice;
    
    std::cout << "\n  ";
    std::cout << colorize("HIGH: $" + highStr.str(), "green");
    std::cout << "  |  ";
    std::cout << colorize("LOW: $" + lowStr.str(), "red");
    std::cout << "  |  OPEN: $" << openStr.str();
    std::cout << "\n";
}

// ============================================================================
// PRICE CHART - ASCII visualization of price history
// ============================================================================

void Visualizer::printPriceChart() const {
    if (m_priceHistory.size() < 2) {
        return;  // Need at least 2 points to draw
    }
    
    const int chartHeight = 12;  // Taller for better resolution
    const int chartWidth = 60;   // Fixed width for consistency
    
    // Find min/max for scaling
    Price minPrice = *std::min_element(m_priceHistory.begin(), m_priceHistory.end());
    Price maxPrice = *std::max_element(m_priceHistory.begin(), m_priceHistory.end());
    
    // Ensure meaningful range (at least 0.5% of price)
    double range = maxPrice - minPrice;
    double avgPrice = (maxPrice + minPrice) / 2.0;
    double minRange = avgPrice * 0.005;  // 0.5% minimum range
    if (range < minRange) {
        double pad = (minRange - range) / 2.0;
        minPrice -= pad;
        maxPrice += pad;
        range = minRange;
    } else {
        // Add 10% padding
        minPrice -= range * 0.1;
        maxPrice += range * 0.1;
        range = maxPrice - minPrice;
    }
    
    // Current price for highlighting
    Price currentPrice = m_priceHistory.back();
    int currentRow = chartHeight - 1 - static_cast<int>(((currentPrice - minPrice) / range) * (chartHeight - 1));
    currentRow = std::max(0, std::min(chartHeight - 1, currentRow));
    
    // Resample price history to chart width for smooth line
    std::vector<int> chartRows(chartWidth);
    for (int col = 0; col < chartWidth; ++col) {
        // Map column to price history index
        size_t histIdx = (col * (m_priceHistory.size() - 1)) / (chartWidth - 1);
        histIdx = std::min(histIdx, m_priceHistory.size() - 1);
        
        double normalized = (m_priceHistory[histIdx] - minPrice) / range;
        int row = chartHeight - 1 - static_cast<int>(normalized * (chartHeight - 1));
        chartRows[col] = std::max(0, std::min(chartHeight - 1, row));
    }
    
    // Header with current price prominently displayed
    std::cout << "\n";
    std::string priceColor = (currentPrice >= m_sessionOpenPrice) ? "green" : "red";
    std::cout << "  " << colorize("PRICE: $", "white");
    std::cout << colorize(std::to_string(static_cast<int>(currentPrice)) + "." + 
        std::to_string(static_cast<int>((currentPrice - static_cast<int>(currentPrice)) * 100)), priceColor);
    
    double changeFromOpen = currentPrice - m_sessionOpenPrice;
    double changePct = (m_sessionOpenPrice > 0) ? (changeFromOpen / m_sessionOpenPrice) * 100.0 : 0.0;
    std::ostringstream changeOss;
    changeOss << std::fixed << std::setprecision(2) << std::showpos << changeFromOpen;
    std::ostringstream pctOss;
    pctOss << std::fixed << std::setprecision(2) << std::showpos << changePct << "%";
    std::cout << "  " << colorize("(" + changeOss.str() + " / " + pctOss.str() + ")", priceColor);
    std::cout << "\n";
    
    // Price scale on left
    std::cout << "  $" << std::fixed << std::setprecision(2) << std::setw(7) << maxPrice;
    std::cout << " +";
    for (int i = 0; i < chartWidth; ++i) std::cout << "-";
    std::cout << "+\n";
    
    // Draw chart rows
    for (int row = 0; row < chartHeight; ++row) {
        // Price label at top, middle, bottom
        if (row == 0) {
            std::cout << "           ";
        } else if (row == chartHeight / 2) {
            double midPrice = (maxPrice + minPrice) / 2.0;
            std::cout << "  $" << std::fixed << std::setprecision(2) << std::setw(7) << midPrice;
        } else if (row == chartHeight - 1) {
            std::cout << "  $" << std::fixed << std::setprecision(2) << std::setw(7) << minPrice;
        } else {
            std::cout << "           ";
        }
        
        std::cout << " |";
        
        // Draw the chart line
        for (int col = 0; col < chartWidth; ++col) {
            bool isLastCol = (col == chartWidth - 1);
            
            if (chartRows[col] == row) {
                // This is a data point
                if (isLastCol) {
                    // Current price - make it very visible
                    std::cout << colorize("@", "yellow");
                } else if (col > 0 && chartRows[col] < chartRows[col-1]) {
                    // Price went up
                    std::cout << colorize("/", "green");
                } else if (col > 0 && chartRows[col] > chartRows[col-1]) {
                    // Price went down
                    std::cout << colorize("\\", "red");
                } else {
                    // Price flat or starting point
                    std::cout << colorize("-", "cyan");
                }
            } else if (col > 0) {
                // Draw connecting lines for smooth appearance
                int prevRow = chartRows[col-1];
                int currRow = chartRows[col];
                if ((row > std::min(prevRow, currRow) && row < std::max(prevRow, currRow))) {
                    // Vertical connector
                    if (currRow < prevRow) {
                        std::cout << colorize("|", "green");
                    } else {
                        std::cout << colorize("|", "red");
                    }
                } else {
                    std::cout << " ";
                }
            } else {
                std::cout << " ";
            }
        }
        
        // Show current price marker on the right
        if (row == currentRow) {
            std::cout << colorize("<< $" + std::to_string(static_cast<int>(currentPrice * 100) / 100.0).substr(0,6), "yellow");
        } else {
            std::cout << "|";
        }
        std::cout << "\n";
    }
    
    // Bottom border
    std::cout << "           +";
    for (int i = 0; i < chartWidth; ++i) std::cout << "-";
    std::cout << "+\n";
    
    // Time axis
    std::cout << "            ";
    std::cout << colorize("^oldest", "white");
    std::cout << std::string(chartWidth - 20, ' ');
    std::cout << colorize("newest^", "white");
    std::cout << "\n";
}

void Visualizer::updatePriceTracking(Price price) const {
    // Store previous for tick direction
    m_previousPrice = m_lastPrice;
    m_lastPrice = price;
    
    // Initialize session open on first price
    if (m_sessionOpenPrice == 0.0) {
        m_sessionOpenPrice = price;
    }
    
    // Track high/low
    if (price > m_sessionHighPrice) {
        m_sessionHighPrice = price;
    }
    if (price < m_sessionLowPrice) {
        m_sessionLowPrice = price;
    }
    
    // Add to price history for chart
    m_priceHistory.push_back(price);
    if (m_priceHistory.size() > MAX_PRICE_HISTORY) {
        m_priceHistory.erase(m_priceHistory.begin());
    }
}

// ============================================================================
// TRADE DISPLAY
// ============================================================================

void Visualizer::addTrade(Price price, Quantity quantity, Side side) {
    TradeRecord record{price, quantity, side};
    m_recentTrades.insert(m_recentTrades.begin(), record);
    
    // Trim to max history
    if (m_recentTrades.size() > MAX_TRADE_HISTORY) {
        m_recentTrades.resize(MAX_TRADE_HISTORY);
    }
    
    // UPDATE PRICE TRACKING FROM ACTUAL TRADE PRICES!
    // This is the key - the "current price" is the LAST TRADE PRICE, not the order book mid
    // Note: updatePriceTracking already handles price history for the chart
    updatePriceTracking(price);
}

void Visualizer::printRecentTrades(size_t count) const {
    if (m_recentTrades.empty()) {
        return;
    }
    
    std::cout << "\n  RECENT TRADES\n";
    printLine('-', 40);
    
    size_t n = std::min(count, m_recentTrades.size());
    for (size_t i = 0; i < n; ++i) {
        const auto& trade = m_recentTrades[i];
        std::string color = (trade.side == Side::BUY) ? "green" : "red";
        std::string arrow = (trade.side == Side::BUY) ? "[BUY ]" : "[SELL]";
        
        std::cout << "  " << colorize(arrow, color) << " ";
        std::cout << formatQuantity(trade.quantity) << " @ ";
        std::cout << colorize("$" + formatPrice(trade.price), color);
        std::cout << "\n";
    }
    
    std::cout << "\n";
}

// ============================================================================
// HELPERS
// ============================================================================

std::string Visualizer::formatPrice(Price price) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << std::setw(m_priceWidth) << price;
    return oss.str();
}

std::string Visualizer::formatQuantity(Quantity qty) const {
    std::ostringstream oss;
    oss << std::setw(m_quantityWidth) << qty;
    return oss.str();
}

std::string Visualizer::colorize(const std::string& text, const std::string& color) const {
    if (!m_colorEnabled) {
        return text;
    }
    
    std::string colorCode;
    if (color == "red") {
        colorCode = "\033[31m";
    } else if (color == "green") {
        colorCode = "\033[32m";
    } else if (color == "yellow") {
        colorCode = "\033[33m";
    } else if (color == "blue") {
        colorCode = "\033[34m";
    } else {
        return text;
    }
    
    return colorCode + text + "\033[0m";
}

void Visualizer::printLine(char c, int width) const {
    std::cout << "  " << std::string(width, c) << "\n";
}

} // namespace orderbook
