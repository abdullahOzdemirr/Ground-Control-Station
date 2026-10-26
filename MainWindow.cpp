#include "MainWindow.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <nlohmann/json.hpp>

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

using json = nlohmann::json;

MainWindow::MainWindow(short port) : port_(port) {
    socket_ = std::make_unique<asio::ip::udp::socket>(
        io_context_, asio::ip::udp::endpoint(asio::ip::udp::v4(), port_)
    );
    start_receive();

    io_thread_ = std::thread([this]() {
        io_context_.run();
    });
}

MainWindow::~MainWindow() {
    io_context_.stop();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

void MainWindow::start_receive() {
    socket_->async_receive_from(
        asio::buffer(recv_buffer_), remote_endpoint_,
        [this](std::error_code ec, std::size_t bytes_transferred) {
            handle_receive(ec, bytes_transferred);
        });
}

void MainWindow::handle_receive(const std::error_code& ec, std::size_t bytes_transferred) {
    if (!ec) {
        std::string data(recv_buffer_.data(), bytes_transferred);
        
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%T");

        TelemetryPacket pkt;
        pkt.timestamp = ss.str();
        pkt.rawData = data;
        pkt.is_command = false;

        try {
            auto j = json::parse(data);
            pkt.altitude = j.value("altitude_m", 0.0f);
            pkt.x_pos = j.value("x_pos_m", 0.0f);
            pkt.y_pos = j.value("y_pos_m", 0.0f);
            pkt.speed = j.value("speed_ms", 0.0f);
            pkt.battery = j.value("battery_pct", 0.0f);
            pkt.roll = j.value("roll_deg", 0.0f);
            pkt.pitch = j.value("pitch_deg", 0.0f);
            pkt.status = j.value("status", "NORMAL");

            current_altitude_.store(pkt.altitude);
            current_x_pos_.store(pkt.x_pos);
            current_y_pos_.store(pkt.y_pos);
            current_speed_.store(pkt.speed);
            current_battery_.store(pkt.battery);
            current_roll_.store(pkt.roll);
            current_pitch_.store(pkt.pitch);
            snprintf(current_status_, sizeof(current_status_), "%s", pkt.status.c_str());
        } catch (...) {}

        {
            std::lock_guard<std::mutex> lock(log_mutex_);
            packet_log_.push_back(pkt);
            total_packets_++;

            altitude_history_.push_back(pkt.altitude);
            speed_history_.push_back(pkt.speed);
            if (altitude_history_.size() > 100) {
                altitude_history_.erase(altitude_history_.begin());
                speed_history_.erase(speed_history_.begin());
            }

            if (packet_log_.size() > 500) {
                packet_log_.erase(packet_log_.begin());
            }
        }
    }
    start_receive();
}

void MainWindow::send_command(const std::string& cmd_json) {
    try {
        asio::ip::udp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(asio::ip::udp::v4(), "127.0.0.1", "8081");
        socket_->send_to(asio::buffer(cmd_json), *endpoints.begin());

        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%T");

        TelemetryPacket cmd_pkt;
        cmd_pkt.timestamp = ss.str();
        cmd_pkt.rawData = "[UPLINK CMD] " + cmd_json;
        cmd_pkt.is_command = true;

        std::lock_guard<std::mutex> lock(log_mutex_);
        packet_log_.push_back(cmd_pkt);

    } catch (const std::exception& e) {
        std::cerr << "Command send error: " << e.what() << std::endl;
    }
}

void MainWindow::run() {
    if (!glfwInit()) return;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1366, 768, "MERGEN GCS - Ground Control Station", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));
            
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | 
                                            ImGuiWindowFlags_NoResize | 
                                            ImGuiWindowFlags_NoMove | 
                                            ImGuiWindowFlags_NoCollapse;

            ImGui::Begin("GCS Main Dashboard", nullptr, window_flags);

            ImGui::Text("=== MERGEN GCS - PROFESSIONAL GROUND CONTROL SYSTEM ===");
            ImGui::SameLine(display_w - 240);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "SYSTEM: %s", current_status_);
            ImGui::Separator();

            float panel_height = 200.0f;

            // Tablo Genişliğini Tam Ekran Yayacak Şekilde Ayarlıyoruz
            ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp;
            if (ImGui::BeginTable("MainLayoutTable", 4, table_flags)) {
                ImGui::TableNextRow();

                // 1. Sütun: Telemetry Card
                ImGui::TableSetColumnIndex(0);
                ImGui::BeginChild("TelemetryCard", ImVec2(-FLT_MIN, panel_height), true);
                ImGui::Text("Port: %d | Packets: %d", port_, total_packets_.load());
                ImGui::Separator();
                ImGui::Text("Altitude (Z): %.1f m", current_altitude_.load());
                ImGui::Text("Pos X       : %.1f m", current_x_pos_.load());
                ImGui::Text("Pos Y       : %.1f m", current_y_pos_.load());
                ImGui::Text("Speed       : %.1f m/s", current_speed_.load());
                ImGui::Text("Roll/Pitch  : %.1f/%.1f deg", current_roll_.load(), current_pitch_.load());
                
                float bat = current_battery_.load();
                ImGui::Text("Battery     : %.1f%%", bat);
                ImVec4 batColor = bat > 30.0f ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, batColor);
                ImGui::ProgressBar(bat / 100.0f, ImVec2(-1, 0), "");
                ImGui::PopStyleColor();
                ImGui::EndChild();

                // 2. Sütun: Command & Manual Axis Control Panel
                ImGui::TableSetColumnIndex(1);
                ImGui::BeginChild("CommandCard", ImVec2(-FLT_MIN, panel_height), true);
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Flight & Axis Control:");
                ImGui::Separator();
                
                if (ImGui::Button("TAKEOFF", ImVec2(-1, 22))) { json c; c["command"] = "TAKEOFF"; send_command(c.dump()); }
                if (ImGui::Button("RTH", ImVec2(-1, 22))) { json c; c["command"] = "RTH"; send_command(c.dump()); }
                if (ImGui::Button("LAND", ImVec2(-1, 22))) { json c; c["command"] = "LAND"; send_command(c.dump()); }
                
                float half_w = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
                if (ImGui::Button("Fwd / Back", ImVec2(half_w, 22))) { json c; c["command"] = "MOVE_FORWARD"; send_command(c.dump()); }
                ImGui::SameLine();
                if (ImGui::Button("Left / Right", ImVec2(half_w, 22))) { json c; c["command"] = "MOVE_LEFT"; send_command(c.dump()); }
                
                if (ImGui::Button("DISARM", ImVec2(-1, 22))) { json c; c["command"] = "DISARM"; send_command(c.dump()); }
                ImGui::EndChild();

                // 3. Sütun: Artificial Horizon HUD PFD
                ImGui::TableSetColumnIndex(2);
                ImGui::BeginChild("HUDCard", ImVec2(-FLT_MIN, panel_height), true);
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Artificial Horizon HUD:");
                ImGui::Separator();
                
                ImDrawList* hud_draw = ImGui::GetWindowDrawList();
                ImVec2 hud_p0 = ImGui::GetCursorScreenPos();
                ImVec2 hud_sz = ImGui::GetContentRegionAvail();
                if (hud_sz.x < 50) hud_sz.x = 50;
                if (hud_sz.y < 50) hud_sz.y = 50;
                ImVec2 hud_p1 = ImVec2(hud_p0.x + hud_sz.x, hud_p0.y + hud_sz.y);

                hud_draw->PushClipRect(hud_p0, hud_p1, true);

                float center_x = hud_p0.x + hud_sz.x * 0.5f;
                float center_y = hud_p0.y + hud_sz.y * 0.5f;

                float roll = current_roll_.load() * 3.14159f / 180.0f;
                float pitch = current_pitch_.load();
                float horizon_y = center_y + (pitch * 3.0f);

                ImVec2 dir(std::cos(roll), std::sin(roll));
                ImVec2 normal(-dir.y, dir.x);

                hud_draw->AddRectFilled(hud_p0, hud_p1, IM_COL32(30, 130, 220, 255));

                ImVec2 gp1 = ImVec2(center_x - 300 * dir.x - 300 * normal.x, horizon_y - 300 * dir.y - 300 * normal.x);
                ImVec2 gp2 = ImVec2(center_x + 300 * dir.x - 300 * normal.x, horizon_y + 300 * dir.y - 300 * normal.x);
                ImVec2 gp3 = ImVec2(center_x + 300 * dir.x + 300 * normal.x, horizon_y + 300 * dir.y + 300 * normal.x);
                ImVec2 gp4 = ImVec2(center_x - 300 * dir.x + 300 * normal.x, horizon_y - 300 * dir.y + 300 * normal.x);
                hud_draw->AddQuadFilled(gp1, gp2, gp3, gp4, IM_COL32(90, 140, 50, 255));

                hud_draw->AddLine(ImVec2(center_x - 200 * dir.x, horizon_y - 200 * dir.y), 
                                   ImVec2(center_x + 200 * dir.x, horizon_y + 200 * dir.y), IM_COL32(255, 255, 255, 255), 2.0f);

                hud_draw->AddLine(ImVec2(center_x - 15, center_y), ImVec2(center_x - 5, center_y), IM_COL32(255, 140, 0, 255), 2.5f);
                hud_draw->AddLine(ImVec2(center_x + 5, center_y), ImVec2(center_x + 15, center_y), IM_COL32(255, 140, 0, 255), 2.5f);

                hud_draw->PopClipRect();
                ImGui::EndChild();

                // 4. Sütun: Top-Down Navigation / Radar Map
                ImGui::TableSetColumnIndex(3);
                ImGui::BeginChild("NavMapCard", ImVec2(-FLT_MIN, panel_height), true);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "Top-Down Nav Map:");
                ImGui::Separator();

                ImDrawList* map_draw = ImGui::GetWindowDrawList();
                ImVec2 map_p0 = ImGui::GetCursorScreenPos();
                ImVec2 map_sz = ImGui::GetContentRegionAvail();
                if (map_sz.x < 50) map_sz.x = 50;
                if (map_sz.y < 50) map_sz.y = 50;
                ImVec2 map_p1 = ImVec2(map_p0.x + map_sz.x, map_p0.y + map_sz.y);

                map_draw->PushClipRect(map_p0, map_p1, true);

                // Radar Arkaplanı ve Izgaralar
                map_draw->AddRectFilled(map_p0, map_p1, IM_COL32(15, 15, 25, 255));
                map_draw->AddRect(map_p0, map_p1, IM_COL32(80, 80, 100, 255));

                float map_center_x = map_p0.x + map_sz.x * 0.5f;
                float map_center_y = map_p0.y + map_sz.y * 0.5f;

                map_draw->AddLine(ImVec2(map_p0.x, map_center_y), ImVec2(map_p1.x, map_center_y), IM_COL32(60, 60, 80, 255));
                map_draw->AddLine(ImVec2(map_center_x, map_p0.y), ImVec2(map_center_x, map_p1.y), IM_COL32(60, 60, 80, 255));
                map_draw->AddCircle(ImVec2(map_center_x, map_center_y), 35.0f, IM_COL32(70, 70, 100, 200), 12);

                // Home Noktası
                map_draw->AddCircleFilled(ImVec2(map_center_x, map_center_y), 4.0f, IM_COL32(255, 255, 0, 255));
                map_draw->AddText(ImVec2(map_center_x + 6.0f, map_center_y - 6.0f), IM_COL32(255, 255, 0, 255), "HOME");

                // İHA Konumu ve Rota Çizgisi
                float x_curr = current_x_pos_.load();
                float y_curr = current_y_pos_.load();
                
                float drone_px_x = map_center_x + (y_curr * 1.5f);
                float drone_px_y = map_center_y - (x_curr * 1.5f);

                if (drone_px_x < map_p0.x + 8) drone_px_x = map_p0.x + 8;
                if (drone_px_x > map_p1.x - 8) drone_px_x = map_p1.x - 8;
                if (drone_px_y < map_p0.y + 8) drone_px_y = map_p0.y + 8;
                if (drone_px_y > map_p1.y - 8) drone_px_y = map_p1.y - 8;

                map_draw->AddLine(ImVec2(map_center_x, map_center_y), ImVec2(drone_px_x, drone_px_y), IM_COL32(100, 100, 255, 200), 1.5f);
                map_draw->AddCircleFilled(ImVec2(drone_px_x, drone_px_y), 5.0f, IM_COL32(255, 50, 50, 255));

                map_draw->PopClipRect();
                ImGui::EndChild();

                ImGui::EndTable();
            }

            ImGui::Separator();

            // Orta Alan: Canlı Grafikler
            if (ImGui::CollapsingHeader("Live Flight Graphs", ImGuiTreeNodeFlags_DefaultOpen)) {
                std::lock_guard<std::mutex> lock(log_mutex_);
                if (!altitude_history_.empty()) {
                    ImGui::PlotLines("Altitude (m)", altitude_history_.data(), (int)altitude_history_.size(), 0, nullptr, 0.0f, 200.0f, ImVec2(0, 80));
                    ImGui::PlotLines("Speed (m/s)", speed_history_.data(), (int)speed_history_.size(), 0, nullptr, 0.0f, 35.0f, ImVec2(0, 80));
                }
            }
            ImGui::Separator();

            // Alt Alan: Log Konsolu
            ImGui::Text("System Logs & Uplink Commands:");
            if (ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 140), true, ImGuiWindowFlags_HorizontalScrollbar)) {
                std::lock_guard<std::mutex> lock(log_mutex_);
                for (const auto& pkt : packet_log_) {
                    if (pkt.is_command) {
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[%s] %s", pkt.timestamp.c_str(), pkt.rawData.c_str());
                    } else {
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.8f, 1.0f), "[%s]", pkt.timestamp.c_str());
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", pkt.rawData.c_str());
                    }
                }
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }
            }
            ImGui::EndChild();

            ImGui::End();
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}