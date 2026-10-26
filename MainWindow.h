#pragma once

#include <asio.hpp>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <mutex>

struct TelemetryPacket {
    std::string timestamp;
    std::string rawData;
    float altitude = 0.0f;
    float x_pos = 0.0f;
    float y_pos = 0.0f;
    float speed = 0.0f;
    float battery = 0.0f;
    float roll = 0.0f;
    float pitch = 0.0f;
    std::string status = "UNKNOWN";
    bool is_command = false;
};

class MainWindow {
public:
    MainWindow(short port);
    ~MainWindow();

    void run();
    void send_command(const std::string& cmd_json);

private:
    void start_receive();
    void handle_receive(const std::error_code& ec, std::size_t bytes_transferred);

    asio::io_context io_context_;
    std::unique_ptr<asio::ip::udp::socket> socket_;
    asio::ip::udp::endpoint remote_endpoint_;
    std::array<char, 1024> recv_buffer_;
    std::thread io_thread_;
    
    std::vector<TelemetryPacket> packet_log_;
    std::mutex log_mutex_;
    std::atomic<int> total_packets_{0};
    short port_;

    std::atomic<float> current_altitude_{0.0f};
    std::atomic<float> current_x_pos_{0.0f};
    std::atomic<float> current_y_pos_{0.0f};
    std::atomic<float> current_speed_{0.0f};
    std::atomic<float> current_battery_{0.0f};
    std::atomic<float> current_roll_{0.0f};
    std::atomic<float> current_pitch_{0.0f};
    char current_status_[64] = "GROUND";
    
    std::vector<float> altitude_history_;
    std::vector<float> speed_history_;
};