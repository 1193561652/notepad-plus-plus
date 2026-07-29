param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,
    [Parameter(Mandatory = $true)]
    [string]$SettingsDirectory,
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,
    [ValidateSet("original", "qt")]
    [string]$ApplicationKind,
    [double]$QtScaleFactor = 1.0
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

public static class UiCaptureNative {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);

    [DllImport("user32.dll")]
    public static extern bool MoveWindow(
        IntPtr hWnd, int x, int y, int width, int height, bool repaint);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool BringWindowToTop(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern IntPtr SetFocus(IntPtr hWnd);

    [DllImport("kernel32.dll")]
    public static extern uint GetCurrentThreadId();

    [DllImport("user32.dll")]
    public static extern bool AttachThreadInput(
        uint attachThreadId, uint attachToThreadId, bool attach);

    [DllImport("user32.dll")]
    public static extern IntPtr GetForegroundWindow();

    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(int x, int y);

    [DllImport("user32.dll")]
    public static extern void mouse_event(
        uint flags, uint dx, uint dy, uint data, UIntPtr extraInfo);

    [DllImport("user32.dll")]
    public static extern void keybd_event(
        byte virtualKey, byte scanCode, uint flags, UIntPtr extraInfo);

    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);

    [DllImport("user32.dll")]
    public static extern IntPtr SendMessage(
        IntPtr hWnd, uint message, UIntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc callback, IntPtr data);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(
        IntPtr hWnd, out uint processId);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassName(
        IntPtr hWnd, StringBuilder className, int maxCount);

    public static string WindowClass(IntPtr hWnd) {
        StringBuilder value = new StringBuilder(256);
        GetClassName(hWnd, value, value.Capacity);
        return value.ToString();
    }

    public static bool ForceForegroundWindow(IntPtr hWnd) {
        uint processId;
        uint targetThread = GetWindowThreadProcessId(hWnd, out processId);
        uint currentThread = GetCurrentThreadId();
        bool attached = currentThread != targetThread &&
            AttachThreadInput(currentThread, targetThread, true);
        try {
            BringWindowToTop(hWnd);
            bool result = SetForegroundWindow(hWnd);
            SetFocus(hWnd);
            return result;
        }
        finally {
            if (attached)
                AttachThreadInput(currentThread, targetThread, false);
        }
    }

    public static IntPtr[] ProcessWindows(int processId) {
        List<IntPtr> windows = new List<IntPtr>();
        EnumWindows(delegate(IntPtr hWnd, IntPtr data) {
            uint owner;
            GetWindowThreadProcessId(hWnd, out owner);
            if (owner == processId && IsWindowVisible(hWnd))
                windows.Add(hWnd);
            return true;
        }, IntPtr.Zero);
        return windows.ToArray();
    }
}
"@

function ConvertTo-SafeName([string]$Value) {
    $name = $Value -replace '[^A-Za-z0-9._-]+', '-'
    $name = $name.Trim('-')
    if ([string]::IsNullOrWhiteSpace($name)) {
        return "unnamed"
    }
    return $name
}

function Save-WindowImage([IntPtr]$Handle, [string]$Name) {
    if ($Handle -eq [IntPtr]::Zero) {
        throw "Cannot capture an empty window handle for $Name."
    }

    $rect = New-Object UiCaptureNative+RECT
    if (-not [UiCaptureNative]::GetWindowRect($Handle, [ref]$rect)) {
        throw "GetWindowRect failed for $Name."
    }

    $width = [Math]::Max(1, $rect.Right - $rect.Left)
    $height = [Math]::Max(1, $rect.Bottom - $rect.Top)
    $bitmap = New-Object System.Drawing.Bitmap($width, $height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        if ($ApplicationKind -eq "qt") {
            $deviceContext = $graphics.GetHdc()
            try {
                if (-not [UiCaptureNative]::PrintWindow(
                    $Handle, $deviceContext, 2)) {
                    throw "PrintWindow failed for $Name."
                }
            }
            finally {
                $graphics.ReleaseHdc($deviceContext)
            }
        }
        else {
            [void][UiCaptureNative]::ForceForegroundWindow($Handle)
            Start-Sleep -Milliseconds 120
            $graphics.CopyFromScreen(
                $rect.Left, $rect.Top, 0, 0,
                (New-Object System.Drawing.Size($width, $height)))
        }
    }
    finally {
        $graphics.Dispose()
    }

    $path = Join-Path $OutputDirectory ((ConvertTo-SafeName $Name) + ".png")
    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
    return $path
}

function Wait-ForMainWindow([System.Diagnostics.Process]$Process) {
    for ($i = 0; $i -lt 100; ++$i) {
        $bestHandle = [IntPtr]::Zero
        $bestArea = 0
        foreach ($handle in [UiCaptureNative]::ProcessWindows($Process.Id)) {
            if ([UiCaptureNative]::WindowClass($handle) -eq
                "ConsoleWindowClass") {
                continue
            }
            $rect = New-Object UiCaptureNative+RECT
            if ([UiCaptureNative]::GetWindowRect($handle, [ref]$rect)) {
                $area = [Math]::Max(0, $rect.Right - $rect.Left) *
                    [Math]::Max(0, $rect.Bottom - $rect.Top)
                if ($area -gt $bestArea) {
                    $bestArea = $area
                    $bestHandle = $handle
                }
            }
        }
        if ($bestHandle -ne [IntPtr]::Zero) {
            return $bestHandle
        }
        Start-Sleep -Milliseconds 100
    }
    throw "Application did not create a main window."
}

function Wait-ForProcessDialog(
    [System.Diagnostics.Process]$Process,
    [IntPtr]$MainHandle
) {
    for ($i = 0; $i -lt 60; ++$i) {
        $bestHandle = [IntPtr]::Zero
        $bestArea = 0
        foreach ($handle in [UiCaptureNative]::ProcessWindows($Process.Id)) {
            if ($handle -eq $MainHandle -or
                [UiCaptureNative]::WindowClass($handle) -eq
                    "ConsoleWindowClass") {
                continue
            }
            $rect = New-Object UiCaptureNative+RECT
            if ([UiCaptureNative]::GetWindowRect($handle, [ref]$rect)) {
                $area = [Math]::Max(0, $rect.Right - $rect.Left) *
                    [Math]::Max(0, $rect.Bottom - $rect.Top)
                if ($area -gt $bestArea) {
                    $bestArea = $area
                    $bestHandle = $handle
                }
            }
        }
        if ($bestHandle -ne [IntPtr]::Zero) {
            return $bestHandle
        }
        Start-Sleep -Milliseconds 100
    }
    throw "Application did not create the expected dialog window."
}

function Select-And-CaptureAutomationItems(
    [IntPtr]$WindowHandle,
    [System.Windows.Automation.ControlType]$ControlType,
    [string]$Prefix,
    [int]$MaximumItems = [int]::MaxValue
) {
    $root = [System.Windows.Automation.AutomationElement]::FromHandle(
        $WindowHandle)
    $condition = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
        $ControlType)
    $items = $root.FindAll(
        [System.Windows.Automation.TreeScope]::Descendants, $condition)

    $captured = 0
    for ($i = 0; $i -lt $items.Count; ++$i) {
        if ($captured -ge $MaximumItems) {
            break
        }
        $item = $items.Item($i)
        $pattern = $null
        if ($item.TryGetCurrentPattern(
            [System.Windows.Automation.SelectionItemPattern]::Pattern,
            [ref]$pattern)) {
            try {
                $pattern.Select()
                Start-Sleep -Milliseconds 180
                $label = $item.Current.Name
                Save-WindowImage $WindowHandle (
                    "$Prefix-{0:D2}-{1}" -f $captured, $label) | Out-Null
                ++$captured
            }
            catch {
                # Some framework-owned items expose the pattern but reject it.
            }
        }
    }
    return $captured
}

function Click-WindowPoint([IntPtr]$Handle, [int]$X, [int]$Y) {
    $rect = New-Object UiCaptureNative+RECT
    [void][UiCaptureNative]::GetWindowRect($Handle, [ref]$rect)
    [void][UiCaptureNative]::ForceForegroundWindow($Handle)
    [void][UiCaptureNative]::SetCursorPos(
        $rect.Left + $X, $rect.Top + $Y)
    [UiCaptureNative]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    [UiCaptureNative]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
}

function Send-KeyChord(
    [IntPtr]$Handle,
    [byte]$Modifier,
    [byte]$Key
) {
    $downMessage = if ($Modifier -eq 0x12) { 0x0104 } else { 0x0100 }
    $upMessage = if ($Modifier -eq 0x12) { 0x0105 } else { 0x0101 }
    [void][UiCaptureNative]::SendMessage(
        $Handle, 0x0100, [UIntPtr]$Modifier, [IntPtr]::Zero)
    [void][UiCaptureNative]::SendMessage(
        $Handle, $downMessage, [UIntPtr]$Key, [IntPtr]::Zero)
    [void][UiCaptureNative]::SendMessage(
        $Handle, $upMessage, [UIntPtr]$Key, [IntPtr]::Zero)
    [void][UiCaptureNative]::SendMessage(
        $Handle, 0x0101, [UIntPtr]$Modifier, [IntPtr]::Zero)
}

function Send-Key([IntPtr]$Handle, [byte]$Key) {
    [void][UiCaptureNative]::SendMessage(
        $Handle, 0x0100, [UIntPtr]$Key, [IntPtr]::Zero)
    [void][UiCaptureNative]::SendMessage(
        $Handle, 0x0101, [UIntPtr]$Key, [IntPtr]::Zero)
}

function Capture-FindTabsFallback([IntPtr]$Handle) {
    if ($ApplicationKind -eq "original") {
        $centers = @(32, 75, 135, 219, 270)
        $y = 42
    }
    else {
        $centers = @(72, 190, 306, 425, 544)
        $y = 47
    }

    for ($i = 0; $i -lt $centers.Count; ++$i) {
        Click-WindowPoint $Handle $centers[$i] $y
        Start-Sleep -Milliseconds 180
        Save-WindowImage $Handle ("find-tab-{0:D2}" -f $i) | Out-Null
    }
}

function Capture-OriginalPreferencePagesFallback([IntPtr]$Handle) {
    for ($i = 0; $i -lt 19; ++$i) {
        Click-WindowPoint $Handle 88 (55 + 15 * $i)
        Start-Sleep -Milliseconds 180
        Save-WindowImage $Handle (
            "preferences-page-{0:D2}" -f $i) | Out-Null
    }
}

$resolvedExecutable = (Resolve-Path -LiteralPath $Executable).Path
$resolvedSettings = (Resolve-Path -LiteralPath $SettingsDirectory).Path
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path

$oldAppData = $env:APPDATA
$oldLocalAppData = $env:LOCALAPPDATA
$oldSmoke = $env:NPPQT_SMOKE_TEST
$oldScale = $env:QT_SCALE_FACTOR

$process = $null
try {
    $arguments = @()
    if ($ApplicationKind -eq "original") {
        $arguments = @(
            "-multiInst",
            "-nosession",
            "-noPlugin",
            "-settingsDir=`"$resolvedSettings`""
        )
    }
    else {
        $appDataRoot = Split-Path -Parent $resolvedSettings
        $env:APPDATA = $appDataRoot
        $env:LOCALAPPDATA = Join-Path $appDataRoot "Local"
        $env:QT_SCALE_FACTOR = [string]::Format(
            [Globalization.CultureInfo]::InvariantCulture,
            "{0:0.##}", $QtScaleFactor)
        Remove-Item Env:NPPQT_SMOKE_TEST -ErrorAction SilentlyContinue
    }

    if ($arguments.Count -gt 0) {
        $process = Start-Process -FilePath $resolvedExecutable `
            -ArgumentList $arguments -PassThru
    }
    else {
        $process = Start-Process -FilePath $resolvedExecutable -PassThru
    }
    $mainHandle = Wait-ForMainWindow $process
    [void][UiCaptureNative]::MoveWindow(
        $mainHandle, 80, 60, 1100, 760, $true)
    [void][UiCaptureNative]::ForceForegroundWindow($mainHandle)
    Start-Sleep -Milliseconds 500

    Save-WindowImage $mainHandle "main" | Out-Null

    $shell = New-Object -ComObject WScript.Shell
    [void]$shell.AppActivate($process.Id)
    [void][UiCaptureNative]::ForceForegroundWindow($mainHandle)
    Click-WindowPoint $mainHandle 500 400
    Start-Sleep -Milliseconds 150
    Send-KeyChord $mainHandle 0x11 0x46
    $findHandle = Wait-ForProcessDialog $process $mainHandle
    Start-Sleep -Milliseconds 300
    Save-WindowImage $findHandle "find-dialog" | Out-Null
    $findTabCount = Select-And-CaptureAutomationItems $findHandle `
        ([System.Windows.Automation.ControlType]::TabItem) "find-tab"
    if ($findTabCount -eq 0) {
        Capture-FindTabsFallback $findHandle
    }
    Send-Key $findHandle 0x1B
    Start-Sleep -Milliseconds 200

    [void]$shell.AppActivate($process.Id)
    [void][UiCaptureNative]::ForceForegroundWindow($mainHandle)
    Click-WindowPoint $mainHandle 500 400
    Start-Sleep -Milliseconds 150
    Send-KeyChord $mainHandle 0x12 0x54
    Start-Sleep -Milliseconds 150
    Send-Key $mainHandle 0x50
    $preferencesHandle = Wait-ForProcessDialog $process $mainHandle
    Start-Sleep -Milliseconds 500
    Save-WindowImage $preferencesHandle "preferences-dialog" | Out-Null
    $preferenceItemCount = Select-And-CaptureAutomationItems $preferencesHandle `
        ([System.Windows.Automation.ControlType]::ListItem) "preferences-page" 19
    $preferenceItemCount += Select-And-CaptureAutomationItems $preferencesHandle `
        ([System.Windows.Automation.ControlType]::TreeItem) "preferences-tree" `
        (19 - $preferenceItemCount)
    if ($preferenceItemCount -eq 0 -and
        $ApplicationKind -eq "original") {
        Capture-OriginalPreferencePagesFallback $preferencesHandle
    }
    Send-Key $preferencesHandle 0x1B
    Start-Sleep -Milliseconds 200
}
finally {
    if ($process -and -not $process.HasExited) {
        [void]$process.CloseMainWindow()
        if (-not $process.WaitForExit(3000)) {
            Stop-Process -Id $process.Id -Force
        }
    }
    $env:APPDATA = $oldAppData
    $env:LOCALAPPDATA = $oldLocalAppData
    $env:NPPQT_SMOKE_TEST = $oldSmoke
    $env:QT_SCALE_FACTOR = $oldScale
}
