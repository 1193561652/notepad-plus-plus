/*
 * Notepad++ for Qt: an independent Qt port of Notepad++ v8.4.6.
 * Qt port Copyright (C) 2026 Jiang Liwei.
 * Port source: https://github.com/1193561652/notepad-plus-plus/tree/qt-port
 * Original source: https://github.com/notepad-plus-plus/notepad-plus-plus
 * License: see ../LICENSE and ../QT_PORT_NOTICE.md.
 */

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMenu>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QSharedPointer>
#include <QTimer>
#include "CommandLineOptions.h"
#include "MainWindow.h"
#include "MISC/UiFont.h"
#include "Parameters.h"

int main(int argc, char *argv[])
{
    QElapsedTimer startupTimer;
    startupTimer.start();
    QApplication app(argc, argv);

    // These identifiers intentionally remain compatible with Notepad++ and
    // earlier Qt-port settings. They are configuration keys, not authorship or
    // project-ownership claims; see QT_PORT_NOTICE.md.
    app.setApplicationName("Notepad++");
    app.setApplicationDisplayName("Notepad++ for Qt");
    app.setApplicationVersion("8.4.6");
    app.setOrganizationName("Notepad++");
    app.setOrganizationDomain("notepad-plus-plus.org");
    app.setFont(notepadPlusPlusUiFont());

    CommandLineOptions commandLine = CommandLineParser::parse(
        QCoreApplication::arguments(), QDir::currentPath());

    // settingsDir 和临时本地化必须在配置加载之前生效。
    NppParameters& params = NppParameters::getInstance();
    if (!commandLine.settingsDirectory.isEmpty() &&
        !params.setUserPathOverride(commandLine.settingsDirectory)) {
        QMessageBox::warning(
            nullptr, QObject::tr("Invalid settings directory"),
            params.configPathError() +
                QObject::tr("\n\nThe command-line override will be ignored."));
    }
    if (!commandLine.localizationCode.isEmpty()) {
        params.setStartupLocalizationFile(
            CommandLineParser::localizationFileName(
                commandLine.localizationCode));
    }
    if (!params.load()) {
        QMessageBox::critical(
            nullptr, QObject::tr("Settings directory error"),
            params.configPathError());
        return 1;
    }

    // 加载界面语言（NativeLangSpeaker 机制，与原版一致）：
    //   NativeLangSpeaker 读取 XML 语言文件，运行时直接应用到 UI 控件，无需重启。
    // 对应原版：nativeLangDocRootA → _nativeLangSpeaker.init() + changeMenuLang()
    params.reloadNativeLang();
    if (commandLine.showHelp) {
        QMessageBox::information(
            nullptr, QObject::tr("Notepad++ Command Argument Help"),
            CommandLineParser::helpText());
        commandLine.showHelp = false;
    }

    bool useNewInstance = commandLine.multiInstance;
    const int multiInstanceSetting = params.getNppGUI()._multiInstSetting;
    if (multiInstanceSetting == 2 ||
        (multiInstanceSetting == 1 && commandLine.openSession)) {
        useNewInstance = true;
    }

    const QByteArray serverSeed =
        QFileInfo(params.getUserPath()).absoluteFilePath().toUtf8();
    const QString serverName = QStringLiteral("notepadpp-qt-") +
        QString::fromLatin1(
            QCryptographicHash::hash(serverSeed, QCryptographicHash::Sha1)
                .toHex().left(20));

    bool existingInstance = false;
    if (!useNewInstance) {
        for (int attempt = 0; attempt < 5; ++attempt) {
            QLocalSocket existing;
            existing.connectToServer(serverName, QIODevice::WriteOnly);
            if (existing.waitForConnected(200)) {
                existingInstance = true;
                const QByteArray payload =
                    QJsonDocument(commandLine.toJson()).toJson(
                        QJsonDocument::Compact);
                existing.write(payload);
                existing.flush();
                existing.waitForBytesWritten(1000);
                existing.disconnectFromServer();
                return 0;
            }
        }
    } else {
        QLocalSocket existing;
        existing.connectToServer(serverName, QIODevice::WriteOnly);
        existingInstance = existing.waitForConnected(200);
        if (existingInstance) {
            existing.disconnectFromServer();
            if (multiInstanceSetting == 2)
                commandLine.noSession = true;
        }
    }

    QLocalServer server;
    bool ownsServer = false;
    if (!existingInstance) {
        QLocalServer::removeServer(serverName);
        server.setSocketOptions(QLocalServer::UserAccessOption);
        ownsServer = server.listen(serverName);
    }

    MainWindow mainWindow(commandLine);
    if (!commandLine.systemTray && !commandLine.quickPrint &&
        !commandLine.exportFunctionList)
        mainWindow.show();

    QSystemTrayIcon tray;
    QMenu trayMenu;
    if (commandLine.systemTray && QSystemTrayIcon::isSystemTrayAvailable()) {
        tray.setIcon(mainWindow.windowIcon());
        QAction* showAction = trayMenu.addAction(QObject::tr("Show Notepad++"));
        QAction* exitAction = trayMenu.addAction(QObject::tr("Exit"));
        QObject::connect(showAction, &QAction::triggered, &mainWindow, [&]() {
            mainWindow.show();
            mainWindow.raise();
            mainWindow.activateWindow();
        });
        QObject::connect(exitAction, &QAction::triggered,
                         &mainWindow, &QWidget::close);
        QObject::connect(&tray, &QSystemTrayIcon::activated, &mainWindow,
                         [&](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger ||
                reason == QSystemTrayIcon::DoubleClick) {
                mainWindow.show();
                mainWindow.raise();
                mainWindow.activateWindow();
            }
        });
        tray.setContextMenu(&trayMenu);
        tray.show();
    } else if (commandLine.systemTray) {
        mainWindow.show();
    }

    if (ownsServer) {
        QObject::connect(&server, &QLocalServer::newConnection,
                         &mainWindow, [&]() {
            while (QLocalSocket* socket = server.nextPendingConnection()) {
                const QSharedPointer<QByteArray> payload(new QByteArray);
                QObject::connect(socket, &QLocalSocket::readyRead, socket,
                                 [socket, payload]() {
                    payload->append(socket->readAll());
                });
                QObject::connect(socket, &QLocalSocket::disconnected,
                                 &mainWindow, [socket, payload, &mainWindow]() {
                    payload->append(socket->readAll());
                    const QJsonDocument document =
                        QJsonDocument::fromJson(*payload);
                    if (document.isObject()) {
                        mainWindow.applyCommandLineInvocation(
                            CommandLineOptions::fromJson(document.object()));
                        mainWindow.show();
                        mainWindow.raise();
                        mainWindow.activateWindow();
                    }
                    socket->deleteLater();
                });
            }
        });
    }

    if (commandLine.showLoadingTime) {
        const qint64 elapsed = startupTimer.elapsed();
        QTimer::singleShot(0, &mainWindow, [&mainWindow, elapsed]() {
            QMessageBox::information(
                &mainWindow, QObject::tr("Loading Time"),
                QObject::tr("Notepad++ loaded in %1 ms.").arg(elapsed));
        });
    }
    if (qEnvironmentVariableIsSet("NPPQT_SMOKE_TEST"))
        QTimer::singleShot(1000, &mainWindow, &QWidget::close);

    const int result = app.exec();
    if (ownsServer)
        QLocalServer::removeServer(serverName);
    return result;
}
