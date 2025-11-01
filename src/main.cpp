#include "gui/MainWindow.h"
#include "core/security/SecureMemory.h"
#include "core/security/FIPSModule.h"
#include "utils/Logger.h"
#include "utils/Config.h"
#include "chat/ChatManager.h"
#include <QApplication>
#include <QQmlApplicationEngine>
#include <iostream>

int main(int argc, char *argv[]) {
    using namespace Balance;

    // Initialize Qt application
    QApplication app(argc, argv);
    app.setOrganizationName("Balance");
    app.setApplicationName("Balance Encryption Toolkit");
    app.setApplicationVersion("1.0.0");

    // Initialize logging
    Logger::instance().setLogLevel(LogLevel::Info);
    Logger::instance().setLogFile("balance.log");
    LOG_INFO("Balance starting up...");

    // Initialize secure memory
    if (!SecureMemory::initialize()) {
        LOG_ERROR("Failed to initialize secure memory");
        return 1;
    }

    // Load configuration
    auto& config = Config::instance();
    config.load("balance.conf");  // Ignore error if file doesn't exist

    // Initialize FIPS if enabled
    bool enableFIPS = config.getBool("enable_fips", false);
    if (enableFIPS) {
        auto fipsResult = FIPSModule::initialize(true);
        if (fipsResult.isError()) {
            LOG_WARNING("FIPS mode initialization failed: " + fipsResult.error().message);
        } else {
            LOG_INFO("FIPS mode enabled");
        }
    }

    // Initialize chat system
    auto chatResult = ChatManager::instance().initialize(config.get("database_path", "./balance.db"));
    if (chatResult.isError()) {
        LOG_ERROR("Failed to initialize chat system: " + chatResult.error().message);
    }

    // Create main window
    MainWindow mainWindow;
    mainWindow.show();

    LOG_INFO("Balance ready");

    // Run application
    int result = app.exec();

    // Cleanup
    SecureMemory::shutdown();
    LOG_INFO("Balance shutdown complete");

    return result;
}
