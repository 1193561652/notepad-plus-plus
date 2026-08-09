#pragma once

#include <QtGlobal>

#ifndef Q_OS_WIN
#error Win32NativeDockHost is available only on Windows.
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <QWidget>

class Win32NativeDockHost final : public QWidget
{
    Q_OBJECT

public:
    explicit Win32NativeDockHost(QWidget* parent = nullptr);
    ~Win32NativeDockHost() override;

    bool attachClient(HWND client);
    void releaseClient();
    HWND clientHandle() const { return _client; }
    HWND hostHandle() const;
    int promotedAncestorCount() const;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;

private:
    void resizeClient();

    HWND _client = nullptr;
    HWND _originalParent = nullptr;
    LONG_PTR _originalStyle = 0;
    LONG_PTR _originalExStyle = 0;
};
