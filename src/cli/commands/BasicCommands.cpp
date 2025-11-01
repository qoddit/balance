#include "../CommandExecutor.h"
#include "../../core/encryption/EncryptionEngine.h"
#include "../../core/encryption/BalanceFormat.h"
#include "../../utils/Logger.h"
#include <filesystem>

namespace Balance {

// Help command
static Result<std::string> helpCommand(const Command& cmd) {
    std::string output = "Balance Encryption Toolkit v1.0\n\n";
    output += "Available commands:\n";

    auto& executor = CommandExecutor::instance();
    auto commands = executor.listCommands();

    for (const auto& cmdName : commands) {
        auto desc = executor.getDescription(cmdName);
        output += "  " + cmdName;
        if (!desc.empty()) {
            output += " - " + desc;
        }
        output += "\n";
    }

    return output;
}

// Version command
static Result<std::string> versionCommand(const Command& cmd) {
    return "Balance v1.0.0 (C++20)\nBuilt with OpenSSL, Qt6, SQLite3\n";
}

// List algorithms command
static Result<std::string> listAlgorithmsCommand(const Command& cmd) {
    std::string output = "Available encryption algorithms:\n\n";

    auto algorithms = EncryptionRegistry::instance().getSupportedAlgorithms();

    for (const auto& algo : algorithms) {
        output += "  " + algo.name + "\n";
        output += "    Description: " + algo.description + "\n";
        output += "    Key size: " + std::to_string(algo.keySize) + " bits\n";
        output += "    FIPS approved: " + std::string(algo.isFIPSApproved ? "Yes" : "No") + "\n\n";
    }

    return output;
}

// Encrypt command
static Result<std::string> encryptCommand(const Command& cmd) {
    if (cmd.args.empty()) {
        return Error("Usage: encrypt <file> [--algorithm <algo>] [--output <path>] [--password <pwd>]", -1);
    }

    std::string inputFile = cmd.args[0];
    std::string algorithm = "balance";
    std::string outputFile = inputFile + ".balance";
    std::string password = "default";  // In real implementation, prompt for password

    // Parse flags
    auto algIt = cmd.flags.find("algorithm");
    if (algIt != cmd.flags.end()) {
        algorithm = algIt->second;
    }

    auto outIt = cmd.flags.find("output");
    if (outIt != cmd.flags.end()) {
        outputFile = outIt->second;
    }

    auto pwdIt = cmd.flags.find("password");
    if (pwdIt != cmd.flags.end()) {
        password = pwdIt->second;
    }

    // Perform encryption
    if (algorithm == "balance") {
        auto result = BalanceFormat::encryptFile(inputFile, outputFile, password);
        if (result.isError()) {
            return Error(result.error().message, result.error().code);
        }
        return "File encrypted successfully: " + outputFile;
    } else {
        return Error("Algorithm not yet implemented: " + algorithm, -1);
    }
}

// Decrypt command
static Result<std::string> decryptCommand(const Command& cmd) {
    if (cmd.args.empty()) {
        return Error("Usage: decrypt <file> [--output <path>] [--password <pwd>]", -1);
    }

    std::string inputFile = cmd.args[0];
    std::string outputFile = inputFile;

    // Remove .balance extension if present
    if (outputFile.ends_with(".balance")) {
        outputFile = outputFile.substr(0, outputFile.length() - 8);
    } else {
        outputFile += ".decrypted";
    }

    std::string password = "default";  // In real implementation, prompt for password

    // Parse flags
    auto outIt = cmd.flags.find("output");
    if (outIt != cmd.flags.end()) {
        outputFile = outIt->second;
    }

    auto pwdIt = cmd.flags.find("password");
    if (pwdIt != cmd.flags.end()) {
        password = pwdIt->second;
    }

    // Perform decryption
    auto result = BalanceFormat::decryptFile(inputFile, outputFile, password);
    if (result.isError()) {
        return Error(result.error().message, result.error().code);
    }

    return "File decrypted successfully: " + outputFile;
}

// Status command
static Result<std::string> statusCommand(const Command& cmd) {
    std::string output = "Balance Status:\n\n";
    output += "  Encryption engines: " + std::to_string(EncryptionRegistry::instance().getSupportedAlgorithms().size()) + "\n";
    output += "  CLI commands: " + std::to_string(CommandExecutor::instance().listCommands().size()) + "\n";
    output += "  System: Ready\n";

    return output;
}

// Register commands
static void registerCommands() {
    auto& executor = CommandExecutor::instance();

    executor.registerCommand("help", helpCommand, "Show available commands");
    executor.registerCommand("version", versionCommand, "Show version information");
    executor.registerCommand("list-algorithms", listAlgorithmsCommand, "List available encryption algorithms");
    executor.registerCommand("encrypt", encryptCommand, "Encrypt a file");
    executor.registerCommand("decrypt", decryptCommand, "Decrypt a file");
    executor.registerCommand("status", statusCommand, "Show system status");

    // Register stubs for other commands
    auto stub = [](const Command& cmd) -> Result<std::string> {
        return "Command not yet implemented: " + cmd.name;
    };

    executor.registerCommand("batch-encrypt", stub, "Encrypt multiple files");
    executor.registerCommand("batch-decrypt", stub, "Decrypt multiple files");
    executor.registerCommand("set-algorithm", stub, "Set default algorithm");
    executor.registerCommand("keygen", stub, "Generate encryption key");
    executor.registerCommand("config", stub, "Configure settings");
    executor.registerCommand("chat", stub, "Open chat window");
    // Add more stub registrations for remaining commands...
}

// Auto-register commands at startup
static struct CommandRegistrar {
    CommandRegistrar() { registerCommands(); }
} registrar;

} // namespace Balance
