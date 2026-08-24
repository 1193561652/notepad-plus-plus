#include "localization.h"

#include <QCoreApplication>
#include <QString>

#include <cstdio>

namespace {

bool check(bool condition, const char* message)
{
    if (!condition)
        std::fprintf(stderr, "FAILED: %s\n", message);
    return condition;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    NativeLangSpeaker speaker;
    bool ok = check(speaker.init(
                        QStringLiteral(NPP_CHINESE_LANGUAGE_SOURCE),
                        QStringLiteral(NPP_ENGLISH_LANGUAGE_SOURCE)),
                    "Chinese native language file failed to load");

    QString title;
    QString message;
    ok &= check(speaker.getDoSaveOrNotStrings(title, message),
                "DoSaveOrNot strings were not loaded");
    ok &= check(title == QString::fromUtf8("保存") &&
                    message.contains(QStringLiteral("$STR_REPLACE$")),
                "localized close-save title/message are incorrect");
    ok &= check(speaker.getDialogItemText(
                    QStringLiteral("DoSaveOrNot"), 6, QString()) ==
                    QString::fromUtf8("是(&Y)"),
                "localized Yes button is incorrect");
    ok &= check(speaker.getDialogItemText(
                    QStringLiteral("DoSaveOrNot"), 7, QString()) ==
                    QString::fromUtf8("否(&N)"),
                "localized No button is incorrect");
    ok &= check(speaker.getDialogItemText(
                    QStringLiteral("DoSaveOrNot"), 2, QString()) ==
                    QString::fromUtf8("取消(&C)"),
                "localized Cancel button is incorrect");
    return ok ? 0 : 1;
}
