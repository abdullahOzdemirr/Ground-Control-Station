#include "MainWindow.h"

int main() {
    // 9000 portunu dinleyen GCS arayüzünü başlatıyoruz
    MainWindow gcsWindow(9000);
    gcsWindow.run();
    
    return 0;
}