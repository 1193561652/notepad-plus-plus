#include "Win32PluginSystem/Win32NativeDockHost.h"

#include <QFocusEvent>
#include <QHideEvent>
#include <QResizeEvent>
#include <QShowEvent>

Win32NativeDockHost::Win32NativeDockHost(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("Win32NativeDockHost"));
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setFocusPolicy(Qt::StrongFocus);
}

Win32NativeDockHost::~Win32NativeDockHost()
{
    releaseClient();
}

bool Win32NativeDockHost::attachClient(HWND client)
{
    if (!client || !IsWindow(client))
        return false;
    if (_client == client)
        return true;
    releaseClient();

    setAttribute(Qt::WA_NativeWindow);
    HWND host = reinterpret_cast<HWND>(winId());
    if (!host || !IsWindow(host))
        return false;
    _client = client;
    _originalParent = GetParent(client);
    _originalStyle = GetWindowLongPtrW(client, GWL_STYLE);
    _originalExStyle = GetWindowLongPtrW(client, GWL_EXSTYLE);
    SetWindowLongPtrW(client, GWL_STYLE,
                      (_originalStyle | WS_CHILD) & ~WS_POPUP);
    SetParent(client, host);
    resizeClient();
    SetWindowPos(client, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
                     | SWP_NOZORDER | SWP_FRAMECHANGED);
    ShowWindow(client, isVisible() ? SW_SHOW : SW_HIDE);
    return GetParent(client) == host;
}

void Win32NativeDockHost::releaseClient()
{
    if (!_client)
        return;
    if (IsWindow(_client)) {
        ShowWindow(_client, SW_HIDE);
        SetParent(_client, _originalParent);
        SetWindowLongPtrW(_client, GWL_STYLE, _originalStyle);
        SetWindowLongPtrW(_client, GWL_EXSTYLE, _originalExStyle);
        SetWindowPos(_client, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
                         | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    _client = nullptr;
    _originalParent = nullptr;
}

HWND Win32NativeDockHost::hostHandle() const
{
    return reinterpret_cast<HWND>(internalWinId());
}

int Win32NativeDockHost::promotedAncestorCount() const
{
    int count = 0;
    for (QWidget* ancestor = parentWidget(); ancestor;
         ancestor = ancestor->parentWidget()) {
        if (ancestor->internalWinId() && !ancestor->isWindow())
            ++count;
    }
    return count;
}

void Win32NativeDockHost::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    resizeClient();
}

void Win32NativeDockHost::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    resizeClient();
    if (_client && IsWindow(_client))
        ShowWindow(_client, SW_SHOW);
}

void Win32NativeDockHost::hideEvent(QHideEvent* event)
{
    if (_client && IsWindow(_client))
        ShowWindow(_client, SW_HIDE);
    QWidget::hideEvent(event);
}

void Win32NativeDockHost::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
    if (_client && IsWindow(_client))
        SetFocus(_client);
}

void Win32NativeDockHost::resizeClient()
{
    if (_client && IsWindow(_client)) {
        RECT bounds{};
        HWND host = hostHandle();
        if (host && GetClientRect(host, &bounds)) {
            SetWindowPos(_client, nullptr, 0, 0,
                         bounds.right - bounds.left,
                         bounds.bottom - bounds.top,
                         SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }
}
