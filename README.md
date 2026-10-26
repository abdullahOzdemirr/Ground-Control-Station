# GCS - Professional Ground Control Station

**GCS**, C++20 ve Dear ImGui kullanılarak geliştirilmiş, havacılık standartlarına odaklanan yüksek performanslı bir Yer Kontrol İstasyonudur (Ground Control Station). İHA'lar için gerçek zamanlı telemetri takibi, uplink komut yönetimi, yapay ufuk (HUD PFD) ve kuşbakışı navigasyon radar haritası barındırır.

---

## 🚀 Key Features / Özellikler

* **Asynchronous UDP Networking:** `Asio` kütüphanesi ile thread-safe, yüksek performanslı çift yönlü UDP haberleşme (Telemetri alımı ve Komut gönderimi).
* **Artificial Horizon HUD (PFD):** Gerçek zamanlı `Roll` (Yatış) ve `Pitch` (Burun) açılarına göre dinamik olarak dönen, merdiven çizgilerine sahip profesyonel yapay ufuk göstergesi.
* **Top-Down Navigation Map:** İHA'nın kalkış noktasına (HOME) ve X-Y eksenlerine göre anlık konumunu gösteren 2D radar haritası.
* **Flight & Axis Control:** Otonom komutlar (`TAKEOFF`, `RTH`, `LAND`, `DISARM`) ve manuel eksen hareket testleri (`MOVE_FORWARD`, `MOVE_LEFT` vb.).
* **Live Telemetry & Graphing:** İrtifa ve hız verilerini anlık çizen interaktif ImGui grafikleri ve detaylı sistem log konsolu.

---

## 🛠️ Tech Stack / Kullanılan Teknolojiler

* **Language:** C++20
* **GUI / Rendering:** Dear ImGui, GLFW, OpenGL 3.3
* **Networking:** Asio (Standalone)
* **Data Parsing:** nlohmann/json
* **Simulator:** Python 3 (UDP Telemetry & Command Loop)

---

## ⚙️ Build & Run / Derleme ve Çalıştırma

1. Projeyi klonlayın

git clone https://github.com/abdullahOzdemirr/Ground-Control-Station.git

2. CMake ile Derleyin

mkdir build
cd build
cmake ..
cmake --build .
cd ..

3. GCS Uygulamasını Çalıştırın

Derlenen çalıştırılabilir dosyayı (GcsCore.exe) başlatarak arayüze bağlanabilirsiniz.
.\build\Debug\GcsCore.exe

4. Python Simülatörünü Başlatın

Telemetri akışını ve komut testlerini gerçekleştirmek için simülatörü çalıştırın:
python iha_simulator.py

📄 License

Bu proje açık kaynaklıdır, eğitim ve mühendislik amaçları için geliştirilmiştir.