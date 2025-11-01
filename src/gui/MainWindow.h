#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QString>

namespace Balance {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void handleCommand();
    void clearOutput();

private:
    void setupUi();
    void executeCommand(const QString& command);

    QTextEdit* outputArea;
    QLineEdit* inputField;
};

} // namespace Balance
