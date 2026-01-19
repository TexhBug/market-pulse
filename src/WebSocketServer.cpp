// ============================================================================
// WEBSOCKET SERVER IMPLEMENTATION
// ============================================================================

#include "WebSocketServer.h"
#include "OrderBook.h"
#include "CandleManager.h"
#include <iostream>
#include <cstring>
#include <algorithm>

// External debug flag from main.cpp
extern bool g_debug;

namespace orderbook {

// Static instance
WebSocketServer* WebSocketServer::s_instance = nullptr;

// ============================================================================
// JSON Builder Implementation
// ============================================================================

std::string JsonBuilder::orderBookToJson(const OrderBook& book) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    auto bids = book.getTopBids(15);
    auto asks = book.getTopAsks(15);
    auto bestBid = book.getBestBid();
    auto bestAsk = book.getBestAsk();
    
    ss << R"({"type":"orderbook","data":{)";
    
    // Bids (already sorted by price descending from getTopBids)
    ss << R"("bids":[)";
    int count = 0;
    for (const auto& bid : bids) {
        if (count > 0) ss << ",";
        ss << R"({"price":)" << bid.first << R"(,"quantity":)" << bid.second << "}";
        count++;
    }
    ss << "],";
    
    // Asks (already sorted by price ascending from getTopAsks)
    ss << R"("asks":[)";
    count = 0;
    for (const auto& ask : asks) {
        if (count > 0) ss << ",";
        ss << R"({"price":)" << ask.first << R"(,"quantity":)" << ask.second << "}";
        count++;
    }
    ss << "],";
    
    // Best prices and spread
    double bestBidPrice = bestBid.value_or(0.0);
    double bestAskPrice = bestAsk.value_or(0.0);
    double spread = (bestAskPrice > 0 && bestBidPrice > 0) ? bestAskPrice - bestBidPrice : 0.0;
    
    ss << R"("bestBid":)" << bestBidPrice << ",";
    ss << R"("bestAsk":)" << bestAskPrice << ",";
    ss << R"("spread":)" << spread;
    
    ss << "}}";
    return ss.str();
}

std::string JsonBuilder::tradeToJson(double price, int quantity, const std::string& side) {
    static int tradeId = 0;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    ss << R"({"type":"trade","data":{)";
    ss << R"("id":)" << ++tradeId << ",";
    ss << R"("price":)" << price << ",";
    ss << R"("quantity":)" << quantity << ",";
    ss << R"("side":")" << side << R"(",)";
    ss << R"("timestamp":)" << std::time(nullptr) * 1000;  // milliseconds
    ss << "}}";
    
    return ss.str();
}

std::string JsonBuilder::statsToJson(
    const std::string& symbol,
    double currentPrice,
    double openPrice,
    double highPrice,
    double lowPrice,
    size_t totalOrders,
    size_t totalTrades,
    size_t totalVolume,
    int marketOrderPct,
    const std::string& sentiment,
    const std::string& intensity,
    double spread,
    double speed,
    bool paused,
    bool newsShockEnabled,
    bool newsShockCooldown,
    int newsShockCooldownRemaining,
    int newsShockActiveRemaining
) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    ss << R"({"type":"stats","data":{)";
    ss << R"("symbol":")" << symbol << R"(",)";
    ss << R"("currentPrice":)" << currentPrice << ",";
    ss << R"("openPrice":)" << openPrice << ",";
    ss << R"("highPrice":)" << highPrice << ",";
    ss << R"("lowPrice":)" << lowPrice << ",";
    ss << R"("totalOrders":)" << totalOrders << ",";
    ss << R"("totalTrades":)" << totalTrades << ",";
    ss << R"("totalVolume":)" << totalVolume << ",";
    ss << R"("marketOrderPct":)" << marketOrderPct << ",";
    ss << R"("sentiment":")" << sentiment << R"(",)";
    ss << R"("intensity":")" << intensity << R"(",)";
    ss << R"("spread":)" << spread << ",";
    ss << R"("speed":)" << speed << ",";
    ss << R"("paused":)" << (paused ? "true" : "false") << ",";
    ss << R"("newsShockEnabled":)" << (newsShockEnabled ? "true" : "false") << ",";
    ss << R"("newsShockCooldown":)" << (newsShockCooldown ? "true" : "false") << ",";
    ss << R"("newsShockCooldownRemaining":)" << newsShockCooldownRemaining << ",";
    ss << R"("newsShockActiveRemaining":)" << newsShockActiveRemaining;
    ss << "}}";
    
    return ss.str();
}

std::string JsonBuilder::priceToJson(double price, int volume) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    ss << R"({"type":"price","data":{)";
    ss << R"("timestamp":)" << ms << ",";
    ss << R"("price":)" << price << ",";
    ss << R"("volume":)" << volume;
    ss << "}}";
    
    return ss.str();
}

std::string JsonBuilder::tickToJson(
    const OrderBook& book,
    const std::string& statsJson,
    double price,
    int volume,
    int64_t timestamp,
    const TradeData* trade,
    const std::map<int, orderbook::Candle>& currentCandles,
    const std::vector<orderbook::CompletedCandle>& completedCandles
) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    // Extract stats data (remove outer wrapper {"type":"stats","data":{...}})
    // We need just the inner {...} part
    std::string statsData = "{}"; // Default empty object
    size_t dataStart = statsJson.find("\"data\":");
    if (dataStart != std::string::npos) {
        size_t start = statsJson.find("{", dataStart + 7);
        if (start != std::string::npos) {
            // Find matching closing brace by counting
            int depth = 0;
            size_t end = start;
            for (size_t i = start; i < statsJson.length(); i++) {
                if (statsJson[i] == '{') depth++;
                else if (statsJson[i] == '}') {
                    depth--;
                    if (depth == 0) {
                        end = i;
                        break;
                    }
                }
            }
            if (end > start) {
                statsData = statsJson.substr(start, end - start + 1);
            }
        }
    }
    
    // Build order book JSON inline
    auto bids = book.getTopBids(15);
    auto asks = book.getTopAsks(15);
    auto bestBid = book.getBestBid();
    auto bestAsk = book.getBestAsk();
    
    ss << R"({"type":"tick","data":{)";
    
    // Order book
    ss << R"("orderbook":{)";
    ss << R"("bids":[)";
    int count = 0;
    for (const auto& bid : bids) {
        if (count > 0) ss << ",";
        ss << R"({"price":)" << bid.first << R"(,"quantity":)" << bid.second << "}";
        count++;
    }
    ss << "],";
    ss << R"("asks":[)";
    count = 0;
    for (const auto& ask : asks) {
        if (count > 0) ss << ",";
        ss << R"({"price":)" << ask.first << R"(,"quantity":)" << ask.second << "}";
        count++;
    }
    ss << "],";
    double bestBidPrice = bestBid.value_or(0.0);
    double bestAskPrice = bestAsk.value_or(0.0);
    double spread = (bestAskPrice > 0 && bestBidPrice > 0) ? bestAskPrice - bestBidPrice : 0.0;
    ss << R"("bestBid":)" << bestBidPrice << ",";
    ss << R"("bestAsk":)" << bestAskPrice << ",";
    ss << R"("spread":)" << spread;
    ss << "},";
    
    // Stats
    ss << R"("stats":)" << statsData << ",";
    
    // Price point
    ss << R"("price":{"timestamp":)" << timestamp << R"(,"price":)" << price << R"(,"volume":)" << volume << "},";
    
    // Current candles (for all timeframes)
    ss << R"("currentCandles":{)";
    bool first = true;
    for (const auto& [tf, candle] : currentCandles) {
        if (!first) ss << ",";
        first = false;
        ss << "\"" << tf << "\":{";
        ss << R"("timestamp":)" << candle.timestamp << ",";
        ss << R"("open":)" << candle.open << ",";
        ss << R"("high":)" << candle.high << ",";
        ss << R"("low":)" << candle.low << ",";
        ss << R"("close":)" << candle.close << ",";
        ss << R"("volume":)" << candle.volume;
        ss << "}";
    }
    ss << "},";
    
    // Completed candles (if any)
    if (!completedCandles.empty()) {
        ss << R"("completedCandles":[)";
        for (size_t i = 0; i < completedCandles.size(); i++) {
            if (i > 0) ss << ",";
            const auto& cc = completedCandles[i];
            ss << R"({"timeframe":)" << cc.timeframe << ",";
            ss << R"("candle":{)";
            ss << R"("timestamp":)" << cc.candle.timestamp << ",";
            ss << R"("open":)" << cc.candle.open << ",";
            ss << R"("high":)" << cc.candle.high << ",";
            ss << R"("low":)" << cc.candle.low << ",";
            ss << R"("close":)" << cc.candle.close << ",";
            ss << R"("volume":)" << cc.candle.volume;
            ss << "}}";
        }
        ss << "],";
    } else {
        ss << R"("completedCandles":null,)";
    }
    
    // Trade (optional)
    if (trade && trade->isValid()) {
        ss << R"("trade":{)";
        ss << R"("id":)" << trade->id << ",";
        ss << R"("price":)" << trade->price << ",";
        ss << R"("quantity":)" << trade->quantity << ",";
        ss << R"("side":")" << trade->side << R"(",)";
        ss << R"("timestamp":)" << trade->timestamp;
        ss << "}";
    } else {
        ss << R"("trade":null)";
    }
    
    ss << "}}";
    
    return ss.str();
}

// Candle history response (for getCandles command)
std::string JsonBuilder::candleHistoryToJson(
    int timeframe,
    const std::vector<orderbook::Candle>& candles,
    const orderbook::Candle* current
) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    ss << R"({"type":"candleHistory","data":{)";
    ss << R"("timeframe":)" << timeframe << ",";
    
    // Cached candles
    ss << R"("candles":[)";
    for (size_t i = 0; i < candles.size(); i++) {
        if (i > 0) ss << ",";
        const auto& c = candles[i];
        ss << "{";
        ss << R"("timestamp":)" << c.timestamp << ",";
        ss << R"("open":)" << c.open << ",";
        ss << R"("high":)" << c.high << ",";
        ss << R"("low":)" << c.low << ",";
        ss << R"("close":)" << c.close << ",";
        ss << R"("volume":)" << c.volume;
        ss << "}";
    }
    ss << "],";
    
    // Current candle
    if (current) {
        ss << R"("current":{)";
        ss << R"("timestamp":)" << current->timestamp << ",";
        ss << R"("open":)" << current->open << ",";
        ss << R"("high":)" << current->high << ",";
        ss << R"("low":)" << current->low << ",";
        ss << R"("close":)" << current->close << ",";
        ss << R"("volume":)" << current->volume;
        ss << "}";
    } else {
        ss << R"("current":null)";
    }
    
    ss << "}}";
    return ss.str();
}

// ============================================================================
// WebSocket Server Implementation
// ============================================================================

// Per-session data - just a simple ID, actual data stored in server
struct PerSessionData {
    uint32_t clientId;
};

// Client ID counter
static std::atomic<uint32_t> g_nextClientId{1};

// HTTP callback for health check endpoint (keeps Render.com free tier awake)
static int http_callback(struct lws* wsi, enum lws_callback_reasons reason,
                         void* /*user*/, void* /*in*/, size_t /*len*/) {
    switch (reason) {
        case LWS_CALLBACK_HTTP: {
            // Respond to any HTTP request with a simple health check
            const char* response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Access-Control-Allow-Origin: *\r\n"
                                   "Content-Length: 15\r\n"
                                   "\r\n"
                                   "{\"status\":\"ok\"}";
            lws_write(wsi, (unsigned char*)response, strlen(response), LWS_WRITE_HTTP);
            // Return -1 to close connection after response
            return -1;
        }
        default:
            break;
    }
    return 0;
}

// Protocol definition - HTTP first for health checks, then WebSocket
static struct lws_protocols protocols[] = {
    {
        "http",         // HTTP protocol for health checks
        http_callback,
        0,              // per-session data size
        0,              // rx buffer size
        0,              // id
        NULL,           // user pointer
        0               // tx packet size
    },
    {
        "lws-minimal",  // WebSocket protocol name
        WebSocketServer::callback,
        sizeof(PerSessionData),
        65536,  // rx buffer size
        0,      // id
        NULL,   // user pointer
        65536   // tx packet size
    },
    {
        NULL, NULL, 0, 0, 0, NULL, 0  // Terminator
    }
};

WebSocketServer::WebSocketServer(int port) : m_port(port) {
    s_instance = this;
}

WebSocketServer::~WebSocketServer() {
    stop();
    s_instance = nullptr;
}

bool WebSocketServer::start() {
    if (m_running) return true;

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    
    info.port = m_port;
    info.iface = "0.0.0.0";  // Explicitly bind to all interfaces - required for cloud deployment
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.options = 0;  // No special options - allow plain WebSocket
    
    // WebSocket keepalive settings to prevent connection timeout
    info.ka_time = 60;         // Start keepalive after 60 seconds of inactivity
    info.ka_probes = 3;        // Send 3 probes before giving up
    info.ka_interval = 10;     // 10 seconds between probes
    info.timeout_secs = 0;     // Disable protocol timeout (we handle our own)

    std::cout << "[Server] Starting WebSocket server on 0.0.0.0:" << m_port << std::endl;
    std::cout.flush();  // Force output immediately

    m_context = lws_create_context(&info);
    if (!m_context) {
        std::cerr << "[Server] [ERROR] Failed to create WebSocket context\n";
        return false;
    }

    m_running = true;
    m_metrics.serverStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    m_serverThread = std::thread(&WebSocketServer::serverThread, this);
    
    std::cout << "WebSocket server started on port " << m_port << "\n";
    return true;
}

void WebSocketServer::stop() {
    if (!m_running) return;
    
    m_running = false;
    
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
    
    if (m_context) {
        lws_context_destroy(m_context);
        m_context = nullptr;
    }
    
    std::cout << "WebSocket server stopped\n";
}

// Maximum connection duration: 60 minutes (to prevent free compute exhaustion)
static constexpr int64_t MAX_CONNECTION_DURATION_MS = 60 * 60 * 1000;
// Connection summary display interval: 30 seconds
static constexpr int64_t SUMMARY_INTERVAL_MS = 30 * 1000;

// Forward declaration of formatDuration (defined later in file)
static std::string formatDuration(int64_t ms);

void WebSocketServer::serverThread() {
    int64_t lastTimeoutCheck = 0;
    int64_t lastSummaryDisplay = 0;
    
    while (m_running) {
        // Service WebSocket events - this handles all callbacks
        lws_service(m_context, 50);  // 50ms timeout
        
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        // Display connection summary every 30 seconds (only if there are active connections)
        if (now - lastSummaryDisplay > SUMMARY_INTERVAL_MS) {
            lastSummaryDisplay = now;
            
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            if (!m_clients.empty()) {
                std::cout << "\n[Server] ======= CONNECTION SUMMARY =======\n";
                std::cout << "[Server] Total connections: " << m_metrics.totalConnections 
                          << " | Active: " << m_clients.size() << "\n";
                std::cout << "[Server] ---------------------------------\n";
                
                for (const auto& [id, client] : m_clients) {
                    int64_t durationMs = now - client.connectedAt;
                    int64_t remainingMs = MAX_CONNECTION_DURATION_MS - durationMs;
                    
                    std::cout << "[Server] Session " << id 
                              << " | IP: " << client.ipAddress
                              << " | Active: " << formatDuration(durationMs)
                              << " | Remaining: " << formatDuration(remainingMs)
                              << "\n";
                }
                std::cout << "[Server] =====================================\n\n";
            }
        }
        
        // Check for connection timeouts every 10 seconds
        if (now - lastTimeoutCheck > 10000) {
            lastTimeoutCheck = now;
            
            std::vector<std::pair<uint32_t, struct lws*>> expiredClients;
            {
                std::lock_guard<std::mutex> lock(m_clientsMutex);
                for (auto& [id, client] : m_clients) {
                    int64_t connectionDuration = now - client.connectedAt;
                    if (connectionDuration >= MAX_CONNECTION_DURATION_MS) {
                        expiredClients.push_back({id, client.wsi});
                    }
                }
            }
            
            // Send timeout notification and close expired connections
            for (auto& [clientId, wsi] : expiredClients) {
                std::cout << "[Session " << clientId << "] Connection timeout (60 min limit reached)\n";
                
                // Send a timeout message before closing
                std::string timeoutMsg = R"({"type":"timeout","message":"Session expired after 60 minutes. Please reconnect to continue."})";
                sendToClient(clientId, timeoutMsg);
                
                // Request close from lws
                lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL,
                    (unsigned char*)"Session timeout", 15);
                lws_callback_on_writable(wsi);
            }
        }
    }
}

void WebSocketServer::broadcast(const std::string& message) {
    // Step 1: Add message to each client's queue (under lock)
    std::vector<struct lws*> clientsToNotify;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (auto& [id, client] : m_clients) {
            // Limit queue size to prevent memory issues
            if (client.messageQueue.size() < 100) {
                client.messageQueue.push(message);
            }
            clientsToNotify.push_back(client.wsi);
        }
    }
    
    // Step 2: Request writable callback OUTSIDE the lock
    for (auto* wsi : clientsToNotify) {
        lws_callback_on_writable(wsi);
    }
    
    // Cancel any wait so lws_service processes sooner
    if (m_context) {
        lws_cancel_service(m_context);
    }
}

void WebSocketServer::broadcastOrderBook(const std::string& json) {
    broadcast(json);
}

void WebSocketServer::broadcastTrade(const std::string& json) {
    broadcast(json);
}

void WebSocketServer::broadcastStats(const std::string& json) {
    broadcast(json);
}

void WebSocketServer::broadcastPrice(const std::string& json) {
    broadcast(json);
}

void WebSocketServer::sendToClient(uint32_t clientId, const std::string& message) {
    struct lws* wsi = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end()) {
            // Limit queue size to prevent memory issues
            if (it->second.messageQueue.size() < 100) {
                it->second.messageQueue.push(message);
                // Track bytes sent
                m_metrics.totalBytesSent += message.size();
                m_metrics.totalMessagesOut++;
            }
            wsi = it->second.wsi;
        }
    }
    
    // Request writable callback outside the lock
    if (wsi) {
        lws_callback_on_writable(wsi);
    }
    
    if (m_context) {
        lws_cancel_service(m_context);
    }
}

std::vector<uint32_t> WebSocketServer::getClientIds() const {
    std::vector<uint32_t> ids;
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (const auto& [id, _] : m_clients) {
        ids.push_back(id);
    }
    return ids;
}

SessionState* WebSocketServer::getSession(uint32_t clientId) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    auto it = m_clients.find(clientId);
    if (it != m_clients.end() && it->second.session) {
        return it->second.session.get();
    }
    return nullptr;
}

const SessionState* WebSocketServer::getSession(uint32_t clientId) const {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    auto it = m_clients.find(clientId);
    if (it != m_clients.end() && it->second.session) {
        return it->second.session.get();
    }
    return nullptr;
}

std::vector<SessionState*> WebSocketServer::getAllSessions() {
    std::vector<SessionState*> sessions;
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (auto& [id, client] : m_clients) {
        if (client.session && client.session->isRunning()) {
            sessions.push_back(client.session.get());
        }
    }
    return sessions;
}

// Helper to format bytes
static std::string formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unit < 3) {
        size /= 1024.0;
        unit++;
    }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return ss.str();
}

// Helper to format duration
static std::string formatDuration(int64_t ms) {
    int64_t seconds = ms / 1000;
    int64_t minutes = seconds / 60;
    int64_t hours = minutes / 60;
    
    std::ostringstream ss;
    if (hours > 0) {
        ss << hours << "h " << (minutes % 60) << "m " << (seconds % 60) << "s";
    } else if (minutes > 0) {
        ss << minutes << "m " << (seconds % 60) << "s";
    } else {
        ss << seconds << "s";
    }
    return ss.str();
}

void WebSocketServer::printStats() const {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t uptime = now - m_metrics.serverStartTime;
    
    std::cout << "\n========================================\n";
    std::cout << "  WebSocket Server Stats (TOTAL)\n";
    std::cout << "========================================\n";
    std::cout << "  Uptime: " << formatDuration(uptime) << "\n";
    std::cout << "  Active Connections: " << m_metrics.activeConnections.load() << "\n";
    std::cout << "  Total Connections: " << m_metrics.totalConnections.load() << "\n";
    std::cout << "  Messages In: " << m_metrics.totalMessagesIn.load() << "\n";
    std::cout << "  Messages Out: " << m_metrics.totalMessagesOut.load() << "\n";
    std::cout << "  Bytes Received: " << formatBytes(m_metrics.totalBytesReceived.load()) << "\n";
    std::cout << "  Bytes Sent: " << formatBytes(m_metrics.totalBytesSent.load()) << "\n";
    std::cout << "========================================\n";
    
    // Also print per-session stats
    printAllSessionStats();
}

void WebSocketServer::printAllSessionStats() const {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    
    if (m_clients.empty()) {
        std::cout << "  No active sessions\n\n";
        return;
    }
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::cout << "\n  Active Sessions:\n";
    std::cout << "  ----------------------------------------\n";
    
    for (const auto& [id, client] : m_clients) {
        int64_t duration = now - client.connectedAt;
        std::cout << "  [Session " << id << "] " << client.ipAddress << "\n";
        std::cout << "    Duration: " << formatDuration(duration) << "\n";
        std::cout << "    Sent: " << formatBytes(client.bytesSent) 
                  << " (" << client.messagesSent << " msgs)\n";
        std::cout << "    Recv: " << formatBytes(client.bytesReceived) 
                  << " (" << client.messagesReceived << " msgs)\n";
    }
    std::cout << "  ----------------------------------------\n\n";
}

std::string WebSocketServer::getSessionStatsString(uint32_t clientId) const {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    
    auto it = m_clients.find(clientId);
    if (it == m_clients.end()) {
        return "Session not found";
    }
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t duration = now - it->second.connectedAt;
    
    std::ostringstream ss;
    ss << "Duration: " << formatDuration(duration)
       << " | Sent: " << formatBytes(it->second.bytesSent) << " (" << it->second.messagesSent << " msgs)"
       << " | Recv: " << formatBytes(it->second.bytesReceived) << " (" << it->second.messagesReceived << " msgs)";
    return ss.str();
}

void WebSocketServer::processMessage(uint32_t clientId, const std::string& message) {
    // Simple JSON parsing for commands
    // Expected formats:
    //   {"type":"sentiment","value":"BULLISH"}
    //   {"type":"start","config":{"symbol":"AAPL","price":180,...}}
    
    if (!m_commandCallback) return;
    
    if (g_debug) {
        std::cout << "[Session " << clientId << "] [DEBUG] Raw: " << message << std::endl;
    }
    
    // Find type
    size_t typePos = message.find("\"type\":");
    if (typePos == std::string::npos) return;
    
    // Extract type
    size_t typeStart = message.find("\"", typePos + 7) + 1;
    size_t typeEnd = message.find("\"", typeStart);
    std::string type = message.substr(typeStart, typeEnd - typeStart);
    
    if (g_debug) {
        std::cout << "[Session " << clientId << "] [DEBUG] Command type: " << type << std::endl;
    }
    
    // Handle "start" command with config object
    if (type == "start") {
        // Parse config object
        size_t configPos = message.find("\"config\":");
        if (configPos != std::string::npos) {
            // Extract individual config values
            auto extractString = [&](const std::string& key) -> std::string {
                size_t pos = message.find("\"" + key + "\":", configPos);
                if (pos == std::string::npos) return "";
                size_t start = message.find("\"", pos + key.length() + 3) + 1;
                size_t end = message.find("\"", start);
                return message.substr(start, end - start);
            };
        
        auto extractNumber = [&](const std::string& key) -> std::string {
            size_t pos = message.find("\"" + key + "\":", configPos);
            if (pos == std::string::npos) return "";
            size_t start = pos + key.length() + 3;
            size_t end = message.find_first_of(",}", start);
            return message.substr(start, end - start);
        };
        
        // Send each config value as separate command
        std::string symbol = extractString("symbol");
        std::string price = extractNumber("price");
        std::string spread = extractNumber("spread");
        std::string sentiment = extractString("sentiment");
        std::string intensity = extractString("intensity");
        std::string speed = extractNumber("speed");
        
        if (g_debug) {
            std::cout << "[Session " << clientId << "] [DEBUG] Config - symbol=" << symbol << " price=" << price 
                      << " spread=" << spread << " sentiment=" << sentiment 
                      << " intensity=" << intensity << " speed=" << speed << std::endl;
        }
        
        if (!symbol.empty()) m_commandCallback(clientId, "symbol", symbol);
        if (!price.empty()) m_commandCallback(clientId, "price", price);
        if (!spread.empty()) m_commandCallback(clientId, "spread", spread);
        if (!sentiment.empty()) m_commandCallback(clientId, "sentiment", sentiment);
        if (!intensity.empty()) m_commandCallback(clientId, "intensity", intensity);
        if (!speed.empty()) m_commandCallback(clientId, "speed", speed);
        }
        
        // Signal start - always send this for "start" type
        if (g_debug) {
            std::cout << "[Session " << clientId << "] [DEBUG] Sending start signal" << std::endl;
        }
        m_commandCallback(clientId, "start", "true");
        return;
    }
    
    // Handle "getCandles" command with timeframe
    if (type == "getCandles") {
        size_t timeframePos = message.find("\"timeframe\":");
        if (timeframePos != std::string::npos) {
            size_t start = timeframePos + 12;
            size_t end = message.find_first_of(",}", start);
            std::string timeframe = message.substr(start, end - start);
            // Remove any quotes if present
            if (!timeframe.empty() && timeframe[0] == '"') {
                timeframe = timeframe.substr(1, timeframe.length() - 2);
            }
            if (g_debug) {
                std::cout << "[Session " << clientId << "] [DEBUG] getCandles timeframe=" << timeframe << std::endl;
            }
            m_commandCallback(clientId, "getCandles", timeframe);
        }
        return;
    }
    
    // Handle simple value commands
    size_t valuePos = message.find("\"value\":");
    if (valuePos == std::string::npos) return;
    
    // Extract value (could be string, number, or boolean)
    size_t valueStart = valuePos + 8;
    std::string value;
    
    if (message[valueStart] == '"') {
        // String value
        valueStart++;
        size_t valueEnd = message.find("\"", valueStart);
        value = message.substr(valueStart, valueEnd - valueStart);
    } else {
        // Number or boolean
        size_t valueEnd = message.find_first_of(",}", valueStart);
        value = message.substr(valueStart, valueEnd - valueStart);
    }
    
    // Handle newsShock specially - pass both type and boolean value
    if (type == "newsShock") {
        // Convert to normalized value
        std::string normalizedValue = (value == "true" || value == "1") ? "true" : "false";
        m_commandCallback(clientId, type, normalizedValue);
        return;
    }
    
    m_commandCallback(clientId, type, value);
}

int WebSocketServer::callback(struct lws* wsi, enum lws_callback_reasons reason,
                              void* user, void* in, size_t len) {
    if (!s_instance) return 0;
    
    // Debug: print all callback reasons with names for important ones
    if (reason != LWS_CALLBACK_GET_THREAD_ID && 
        reason != LWS_CALLBACK_EVENT_WAIT_CANCELLED &&
        reason != LWS_CALLBACK_SERVER_WRITEABLE) {
        const char* reasonName = "";
        switch(reason) {
            case 0: reasonName = "ESTABLISHED"; break;
            case 4: reasonName = "CLOSED"; break;
            case 6: reasonName = "RECEIVE"; break;
            case 11: reasonName = "SERVER_WRITEABLE"; break;
            case 78: reasonName = "WS_PEER_INITIATED_CLOSE"; break;
            default: reasonName = ""; break;
        }
        if (g_debug) {
            std::cout << "WebSocket callback: reason=" << reason << " " << reasonName << std::endl;
        }
    }
    
    PerSessionData* pss = (PerSessionData*)user;
    
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: {
            // Assign client ID
            uint32_t clientId = g_nextClientId++;
            if (pss) {
                pss->clientId = clientId;
            }
            
            // Get client IP address
            char ipBuffer[128] = {0};
            char nameBuffer[128] = {0};
            lws_get_peer_addresses(wsi, lws_get_socket_fd(wsi), 
                                   nameBuffer, sizeof(nameBuffer),
                                   ipBuffer, sizeof(ipBuffer));
            std::string clientIp = (strlen(ipBuffer) > 0) ? ipBuffer : "unknown";
            
            // Get connection timestamp
            auto now = std::chrono::system_clock::now();
            int64_t connectedAt = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            
            {
                std::lock_guard<std::mutex> lock(s_instance->m_clientsMutex);
                // Create client data with new session state
                ClientData clientData;
                clientData.wsi = wsi;
                clientData.session = std::make_unique<SessionState>(clientId);
                clientData.ipAddress = clientIp;
                clientData.connectedAt = connectedAt;
                clientData.bytesSent = 0;
                clientData.bytesReceived = 0;
                clientData.messagesSent = 0;
                clientData.messagesReceived = 0;
                s_instance->m_clients[clientId] = std::move(clientData);
                s_instance->m_connectionCount++;
                s_instance->m_metrics.totalConnections++;
                s_instance->m_metrics.activeConnections++;
            }
            std::cout << "[Session " << clientId << "] [CONNECT] IP: " << clientIp 
                      << " (active: " << s_instance->m_connectionCount << ")\\n";
            break;
        }
        
        case LWS_CALLBACK_CLOSED: {
            uint32_t clientId = pss ? pss->clientId : 0;
            std::string disconnectInfo;
            {
                std::lock_guard<std::mutex> lock(s_instance->m_clientsMutex);
                auto it = s_instance->m_clients.find(clientId);
                if (it != s_instance->m_clients.end()) {
                    // Calculate session duration
                    auto now = std::chrono::system_clock::now();
                    int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()).count();
                    int64_t durationMs = nowMs - it->second.connectedAt;
                    int durationSec = static_cast<int>(durationMs / 1000);
                    
                    // Format bytes
                    auto formatBytes = [](size_t bytes) -> std::string {
                        if (bytes < 1024) return std::to_string(bytes) + "B";
                        if (bytes < 1024*1024) return std::to_string(bytes/1024) + "KB";
                        return std::to_string(bytes/(1024*1024)) + "MB";
                    };
                    
                    std::ostringstream ss;
                    ss << "IP: " << it->second.ipAddress
                       << " | Duration: " << durationSec << "s"
                       << " | Sent: " << formatBytes(it->second.bytesSent) 
                       << " (" << it->second.messagesSent << " msgs)"
                       << " | Recv: " << formatBytes(it->second.bytesReceived)
                       << " (" << it->second.messagesReceived << " msgs)";
                    disconnectInfo = ss.str();
                    
                    // Session is automatically destroyed with unique_ptr
                    s_instance->m_clients.erase(it);
                }
                s_instance->m_connectionCount--;
                s_instance->m_metrics.activeConnections--;
            }
            std::cout << "[Session " << clientId << "] [DISCONNECT] " << disconnectInfo 
                      << " (active: " << s_instance->m_connectionCount << ")\\n";
            break;
        }
        
        case LWS_CALLBACK_RECEIVE: {
            uint32_t clientId = pss ? pss->clientId : 0;
            std::string message((char*)in, len);
            
            // Track bytes received (global and per-session)
            s_instance->m_metrics.totalBytesReceived += len;
            s_instance->m_metrics.totalMessagesIn++;
            
            {
                std::lock_guard<std::mutex> lock(s_instance->m_clientsMutex);
                auto it = s_instance->m_clients.find(clientId);
                if (it != s_instance->m_clients.end()) {
                    it->second.bytesReceived += len;
                    it->second.messagesReceived++;
                }
            }
            
            if (g_debug) {
                std::cout << "[Session " << clientId << "] [DEBUG] RECEIVE len=" << len << std::endl;
                std::cout << "[Session " << clientId << "] [DEBUG] Raw: " << message << std::endl;
            }
            s_instance->processMessage(clientId, message);
            break;
        }
        
        case LWS_CALLBACK_SERVER_WRITEABLE: {
            if (!pss) break;
            
            uint32_t clientId = pss->clientId;
            std::string msg;
            bool hasMore = false;
            size_t msgLen = 0;
            
            {
                std::lock_guard<std::mutex> lock(s_instance->m_clientsMutex);
                auto it = s_instance->m_clients.find(clientId);
                if (it != s_instance->m_clients.end() && !it->second.messageQueue.empty()) {
                    msg = it->second.messageQueue.front();
                    it->second.messageQueue.pop();
                    hasMore = !it->second.messageQueue.empty();
                    msgLen = msg.length();
                }
            }
            
            if (!msg.empty()) {
                // Prepare buffer with LWS_PRE padding
                std::vector<unsigned char> buf(LWS_PRE + msg.length());
                memcpy(&buf[LWS_PRE], msg.c_str(), msg.length());
                
                int written = lws_write(wsi, &buf[LWS_PRE], msg.length(), LWS_WRITE_TEXT);
                if (written < 0) {
                    std::cerr << "[Session " << clientId << "] [ERROR] Write failed\\n";
                } else {
                    // Track bytes sent (global and per-session)
                    s_instance->m_metrics.totalBytesSent += msgLen;
                    s_instance->m_metrics.totalMessagesOut++;
                    
                    std::lock_guard<std::mutex> lock(s_instance->m_clientsMutex);
                    auto it = s_instance->m_clients.find(clientId);
                    if (it != s_instance->m_clients.end()) {
                        it->second.bytesSent += msgLen;
                        it->second.messagesSent++;
                    }
                }
                
                // If more messages in queue, request another callback
                if (hasMore) {
                    lws_callback_on_writable(wsi);
                }
            }
            break;
        }
        
        // Accept all protocol connections
        case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
            if (g_debug) {
                std::cout << "[Server] [DEBUG] Accepting protocol connection\\n";
            }
            return 0;  // Return 0 to accept
        
        default:
            break;
    }
    
    return 0;
}

} // namespace orderbook
