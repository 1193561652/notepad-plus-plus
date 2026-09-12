#pragma once
#include "Plugin.h"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QElapsedTimer>

namespace QtPlugin {
// A persistent process preserves the managed core's static state between commands.
class ManagedEngine {
    QProcess process;
    QByteArray pending;
public:
    ~ManagedEngine() { stop(); }
    void stop() {
        process.closeWriteChannel();
        if (process.state() != QProcess::NotRunning && !process.waitForFinished(1000)) {
            process.kill(); process.waitForFinished(1000);
        }
        pending.clear();
    }
    void start(const QString& assembly) {
        stop();
        const bool framework = assembly.endsWith(".exe");
        const auto runtime = QStandardPaths::findExecutable(QStringLiteral("dotnet"));
        if (!framework && runtime.isEmpty()) throw std::runtime_error("The .NET runtime is required to run this plugin's original C# engine.");
        if (!QFileInfo::exists(assembly)) throw std::runtime_error("The plugin engine is missing. Install the complete plugin directory.");
        process.setProgram(framework ? assembly : runtime); process.setArguments(framework ? QStringList() : QStringList{assembly});
        process.setProcessChannelMode(QProcess::SeparateChannels);
        process.start();
        if (!process.waitForStarted(5000)) throw std::runtime_error("Cannot start the plugin engine.");
    }
    QJsonValue call(const QJsonObject& request) {
        if (process.state() == QProcess::NotRunning) throw std::runtime_error("The plugin engine is not running; reload the plugin.");
        const auto bytes = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
        if (process.write(bytes) != bytes.size()) throw std::runtime_error("Cannot write to the plugin engine.");
        if (process.bytesToWrite() && !process.waitForBytesWritten(5000)) throw std::runtime_error("Plugin engine input timed out.");
        QElapsedTimer elapsed; elapsed.start();
        while (!pending.contains('\n')) {
            if (!process.waitForReadyRead(qMax(1, 30000 - int(elapsed.elapsed())))) {
                const auto details = process.readAllStandardError();
                stop();
                throw std::runtime_error(("Plugin engine stopped or timed out. " + details).constData());
            }
            pending += process.readAllStandardOutput();
        }
        const int end = pending.indexOf('\n');
        QJsonParseError error;
        const auto response = QJsonDocument::fromJson(pending.left(end), &error).object();
        pending.remove(0, end + 1);
        if (error.error != QJsonParseError::NoError) throw std::runtime_error("Invalid plugin engine response.");
        if (!response.value("ok").toBool()) throw std::runtime_error(response.value("error").toString().toUtf8().constData());
        return response.value("result");
    }
};
}
