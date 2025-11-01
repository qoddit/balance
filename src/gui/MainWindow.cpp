#include "MainWindow.h"
#include "../cli/CommandParser.h"
#include "../cli/CommandExecutor.h"
#include "../cli/CommandHistory.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QApplication>

namespace Balance {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
    setWindowTitle("Balance - Encryption Toolkit");
    resize(900, 600);

    // Set window style for glass-like appearance
    setStyleSheet(R"(
        QMainWindow {
            background-color: rgba(240, 240, 245, 0.95);
        }
        QTextEdit {
            background-color: rgba(255, 255, 255, 0.8);
            border: 1px solid rgba(200, 200, 200, 0.5);
            border-radius: 5px;
            padding: 10px;
            font-family: 'Courier New', monospace;
            font-size: 11pt;
        }
        QLineEdit {
            background-color: rgba(255, 255, 255, 0.9);
            border: 2px solid rgba(100, 100, 255, 0.5);
            border-radius: 5px;
            padding: 8px;
            font-family: 'Courier New', monospace;
            font-size: 11pt;
        }
    )");
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    // Title label
    QLabel* titleLabel = new QLabel("Balance Encryption Toolkit", this);
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #333; padding: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    // Algorithm selector (simplified)
    QLabel* algoLabel = new QLabel("Available algorithms: Balance, AES-256, RSA-4096, Serpent, Blowfish", this);
    algoLabel->setStyleSheet("font-size: 10pt; color: #666; padding: 5px;");
    algoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(algoLabel);

    // Output area
    outputArea = new QTextEdit(this);
    outputArea->setReadOnly(true);
    outputArea->append("Welcome to Balance - Encryption Toolkit v1.0");
    outputArea->append("Type 'help' for available commands");
    outputArea->append("");
    layout->addWidget(outputArea, 1);

    // Input field
    inputField = new QLineEdit(this);
    inputField->setPlaceholderText("Enter command...");
    connect(inputField, &QLineEdit::returnPressed, this, &MainWindow::handleCommand);
    layout->addWidget(inputField);

    // Menu bar
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* clearAction = new QAction("Clear Output", this);
    connect(clearAction, &QAction::triggered, this, &MainWindow::clearOutput);
    fileMenu->addAction(clearAction);

    fileMenu->addSeparator();

    QAction* exitAction = new QAction("E&xit", this);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(exitAction);

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    QAction* aboutAction = new QAction("&About", this);
    helpMenu->addAction(aboutAction);
}

void MainWindow::handleCommand() {
    QString commandText = inputField->text().trimmed();
    if (commandText.isEmpty()) {
        return;
    }

    // Add to history
    CommandHistory::instance().add(commandText.toStdString());

    // Display command
    outputArea->append("$ " + commandText);

    // Clear input
    inputField->clear();

    // Execute command
    executeCommand(commandText);
}

void MainWindow::executeCommand(const QString& command) {
    // Parse command
    auto parseResult = CommandParser::parse(command.toStdString());
    if (parseResult.isError()) {
        outputArea->append("Error: " + QString::fromStdString(parseResult.error().message));
        outputArea->append("");
        return;
    }

    // Execute command
    auto execResult = CommandExecutor::instance().execute(parseResult.value());
    if (execResult.isError()) {
        outputArea->append("Error: " + QString::fromStdString(execResult.error().message));
    } else {
        outputArea->append(QString::fromStdString(execResult.value()));
    }

    outputArea->append("");
}

void MainWindow::clearOutput() {
    outputArea->clear();
    outputArea->append("Output cleared");
    outputArea->append("");
}

} // namespace Balance
