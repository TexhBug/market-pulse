// ============================================================================
// WEBSOCKET SERVER - WebSocket server for frontend communication
// ============================================================================
// Uses libwebsockets to broadcast order book data and receive commands
// ============================================================================

#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <libwebsockets.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <sstream>
#include <iomanip>
#include <memory>
#include <chrono>
#include "CandleManager.h"
#include "SessionState.h"

namespace orderbook {

// Forward declarations
class OrderBook;

// ============================================================================
// JSON Builder Helper
// ============================================================================

class JsonBuilder {
public:
    static std::string orderBookToJson(const OrderBook& book);
    static std::string tradeToJson(double price, int quantity, const std::string& side);
    static std::string statsToJson(
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
        bool newsShockEnabled = false,
        bool newsShockCooldown = false,
        int newsShockCooldownRemaining = 0,
        int newsShockActiveRemaining = 0
    );
    static std::string priceToJson(double price, int volume);
    
    // New batched tick message format (matching Node.js)
    static std::string tickToJson(
        const OrderBook& book,
        const std::string& statsJson,
        double price,
        int volume,
        int64_t timestamp,
        const TradeData* trade,  // nullptr if no trade this tick
        const std::map<int, orderbook::Candle>& currentCandles,
        const std::vector<orderbook::CompletedCandle>& completedCandles
    );
    
    // Candle history response
    static std::string candleHistoryToJson(
        int timeframe,
        const std::vector<orderbook::Candle>& candles,
        const orderbook::Candle* current
    );
};

// ============================================================================
// WebSocket Server
// ============================================================================

// Connection metrics structure
struct ConnectionMetrics {
    std::atomic<size_t> totalConnections{0};
    std::atomic<size_t> activeConnections{0};
    std::atomic<size_t> totalBytesSent{0};
    std::atomic<size_t> totalBytesReceived{0};
    std::atomic<size_t> totalMessagesIn{0};
    std::atomic<size_t> totalMessagesOut{0};
    int64_t serverStartTime{0};
};

class WebSocketServer {
public:
    // Callback now includes client ID for session-specific handling
    using CommandCallback = std::function<void(uint32_t clientId, const std::string& type, const std::string& value)>;

    WebSocketServer(int port = 8080);
    ~WebSocketServer();

    // Start/stop server
    bool start();
    void stop();
    bool isRunning() const { return m_running; }

    // Broadcast messages to all connected clients
    void broadcast(const std::string& message);
    void broadcastOrderBook(const std::string& json);
    void broadcastTrade(const std::string& json);
    void broadcastStats(const std::string& json);
    void broadcastPrice(const std::string& json);
    
    // Send to specific client
    void sendToClient(uint32_t clientId, const std::string& message);

    // Set callback for received commands
    void setCommandCallback(CommandCallback callback) { m_commandCallback = callback; }

    // Get connection count
    size_t getConnectionCount() const { return m_connectionCount; }
    
    // Get connection metrics
    const ConnectionMetrics& getMetrics() const { return m_metrics; }
    
    // Print connection stats
    void printStats() const;
    
    // Print all active session stats
    void printAllSessionStats() const;
    
    // Get session stats string for a specific client
    std::string getSessionStatsString(uint32_t clientId) const;
    
    // Get list of connected client IDs
    std::vector<uint32_t> getClientIds() const;
    
    // Session management - each client has its own session
    SessionState* getSession(uint32_t clientId);
    const SessionState* getSession(uint32_t clientId) const;
    std::vector<SessionState*> getAllSessions();

    // libwebsockets callback (public for C linkage)
    static int callback(struct lws* wsi, enum lws_callback_reasons reason,
                       void* user, void* in, size_t len);

private:
    // Server thread function
    void serverThread();

    // Process incoming message (with client ID)
    void processMessage(uint32_t clientId, const std::string& message);

    int m_port;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_hasData{false};
    std::atomic<size_t> m_connectionCount{0};
    std::thread m_serverThread;
    
    struct lws_context* m_context = nullptr;
    
    // Per-client data structure with session state and metrics
    struct ClientData {
        struct lws* wsi;
        std::queue<std::string> messageQueue;
        std::unique_ptr<SessionState> session;
        
        // Connection info
        std::string ipAddress;
        std::string userAgent;
        int64_t connectedAt;
        
        // Per-session metrics
        size_t bytesSent = 0;
        size_t bytesReceived = 0;
        size_t messagesSent = 0;
        size_t messagesReceived = 0;
    };
    
    // Connected clients mapped by ID
    std::map<uint32_t, ClientData> m_clients;
    mutable std::mutex m_clientsMutex;

    CommandCallback m_commandCallback;
    
    // Connection metrics
    ConnectionMetrics m_metrics;

    // Static instance for callback access
    static WebSocketServer* s_instance;
};

} // namespace orderbook

#endif // WEBSOCKET_SERVER_H
