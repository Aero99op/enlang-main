# =====================================================================
#   Enlangg Sovereign Language - Professional GUI Setup Wizard
#   Runs on any Windows 10/11 system using native .NET WPF (Zero external deps)
# =====================================================================

param(
    [switch]$Silent = $false,
    [string]$CustomPath = ""
)

Add-Type -AssemblyName PresentationFramework
Add-Type -AssemblyName PresentationCore
Add-Type -AssemblyName WindowsBase
Add-Type -AssemblyName System.Windows.Forms

# Win32 API to broadcast environment changes immediately across Windows
$Win32Native = @"
using System;
using System.Runtime.InteropServices;

public class Win32 {
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessageTimeout(
        IntPtr hWnd, uint Msg, UIntPtr wParam, string lParam,
        uint fuFlags, uint uTimeout, out UIntPtr lpdwResult
    );
    public static readonly IntPtr HWND_BROADCAST = new IntPtr(0xffff);
    public const uint WM_SETTINGCHANGE = 0x001A;
    public const uint SMTO_ABORTIFHUNG = 0x0002;
}
"@
Add-Type -TypeDefinition $Win32Native -ErrorAction SilentlyContinue

$DefaultInstallBase = if ($CustomPath) { $CustomPath } else { Join-Path $HOME ".enlangg" }

function Install-EnlanggCore($targetBase, $addToPath, $associateFiles, $statusCallback, $progressCallback) {
    $binDir = Join-Path $targetBase "bin"
    if (!(Test-Path $binDir)) {
        New-Item -ItemType Directory -Force -Path $binDir | Out-Null
    }

    & $statusCallback "Extracting and verifying core binaries..."
    & $progressCallback 25

    $enlanggExe = Join-Path $binDir "enlangg.exe"
    $enlngExe = Join-Path $binDir "enlng.exe"

    $scriptDir = $PSScriptRoot
    $localEnlangg = Join-Path $scriptDir "enlangg.exe"
    $localEnlng = Join-Path $scriptDir "enlng.exe"

    if ((Test-Path $localEnlangg) -and (Test-Path $localEnlng)) {
        Copy-Item -Force $localEnlangg $enlanggExe
        Copy-Item -Force $localEnlng $enlngExe
    } else {
        & $statusCallback "Fetching binaries from GitHub repository..."
        $repoUrl = "https://raw.githubusercontent.com/Aero99op/enlang-main/main"
        $releaseUrl = "https://github.com/Aero99op/enlang-main/releases/latest/download"
        try {
            Invoke-WebRequest -Uri "$repoUrl/enlangg.exe" -OutFile $enlanggExe -UseBasicParsing
            Invoke-WebRequest -Uri "$repoUrl/enlng.exe" -OutFile $enlngExe -UseBasicParsing
        } catch {
            Invoke-WebRequest -Uri "$releaseUrl/enlangg.exe" -OutFile $enlanggExe -UseBasicParsing
            Invoke-WebRequest -Uri "$releaseUrl/enlng.exe" -OutFile $enlngExe -UseBasicParsing
        }
    }

    & $progressCallback 55

    # Configure PATH automatically
    if ($addToPath) {
        & $statusCallback "Configuring system Environment Variables (PATH)..."
        $regKey = "HKCU:\Environment"
        $currentPath = (Get-ItemProperty -Path $regKey -Name Path -ErrorAction SilentlyContinue).Path
        if (-not $currentPath) { $currentPath = "" }
        
        $pathArray = $currentPath -split ";" | Where-Object { $_ -ne "" }
        if ($pathArray -notcontains $binDir) {
            $newPath = if ($currentPath) { "$currentPath;$binDir" } else { $binDir }
            Set-ItemProperty -Path $regKey -Name Path -Value $newPath
        }

        # Update current process PATH
        if ($env:Path -split ";" -notcontains $binDir) {
            $env:Path = "$env:Path;$binDir"
        }

        # Broadcast change to all Windows processes
        $result = [UIntPtr]::Zero
        [Win32]::SendMessageTimeout([Win32]::HWND_BROADCAST, [Win32]::WM_SETTINGCHANGE, [UIntPtr]::Zero, "Environment", [Win32]::SMTO_ABORTIFHUNG, 3000, [ref]$result) | Out-Null
    }

    & $progressCallback 80

    # Optional File Associations
    if ($associateFiles) {
        & $statusCallback "Registering .enlng file associations..."
        try {
            cmd /c "assoc .enlng=EnlanggScript >nul 2>&1"
            cmd /c "ftype EnlanggScript=\"$enlanggExe\" run \"%1\" %* >nul 2>&1"
        } catch {}
    }

    & $progressCallback 100
    & $statusCallback "Enlangg Sovereign Toolchain installed successfully!"
}

# --- Headless Silent Mode ---
if ($Silent) {
    Install-EnlanggCore $DefaultInstallBase $true $true { param($msg) Write-Host $msg } { param($p) }
    exit 0
}

# --- Professional WPF GUI Mode ---
[xml]$xaml = @"
<Window xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="Enlangg Sovereign Toolchain Setup" Height="480" Width="620"
        WindowStartupLocation="CenterScreen" ResizeMode="NoResize"
        Background="#080C14" FontFamily="Segoe UI">
    <Window.Resources>
        <Style TargetType="Button">
            <Setter Property="Foreground" Value="#FFFFFF"/>
            <Setter Property="FontWeight" Value="SemiBold"/>
            <Setter Property="FontSize" Value="13"/>
            <Setter Property="Padding" Value="14,8"/>
            <Setter Property="BorderThickness" Value="1"/>
            <Setter Property="BorderBrush" Value="#2A354D"/>
            <Setter Property="Background" Value="#151D2E"/>
            <Setter Property="Cursor" Value="Hand"/>
            <Setter Property="Template">
                <Setter.Value>
                    <ControlTemplate TargetType="Button">
                        <Border x:Name="border" Background="{TemplateBinding Background}" 
                                BorderBrush="{TemplateBinding BorderBrush}" BorderThickness="{TemplateBinding BorderThickness}" 
                                CornerRadius="6" Padding="{TemplateBinding Padding}">
                            <ContentPresenter HorizontalAlignment="Center" VerticalAlignment="Center"/>
                        </Border>
                        <ControlTemplate.Triggers>
                            <Trigger Property="IsMouseOver" Value="True">
                                <Setter TargetName="border" Property="Background" Value="#1E293B"/>
                                <Setter TargetName="border" Property="BorderBrush" Value="#00F0FF"/>
                            </Trigger>
                        </ControlTemplate.Triggers>
                    </ControlTemplate>
                </Setter.Value>
            </Setter>
        </Style>
        <Style TargetType="CheckBox">
            <Setter Property="Foreground" Value="#CBD5E1"/>
            <Setter Property="FontSize" Value="12.5"/>
            <Setter Property="Margin" Value="0,6,0,0"/>
            <Setter Property="Cursor" Value="Hand"/>
        </Style>
    </Window.Resources>

    <Grid Margin="24">
        <Grid.RowDefinitions>
            <RowDefinition Height="Auto"/>
            <RowDefinition Height="*"/>
            <RowDefinition Height="Auto"/>
        </Grid.RowDefinitions>

        <!-- Header -->
        <Grid Grid.Row="0" Margin="0,0,0,20">
            <Grid.ColumnDefinitions>
                <ColumnDefinition Width="Auto"/>
                <ColumnDefinition Width="*"/>
            </Grid.ColumnDefinitions>
            <Border Grid.Column="0" Width="52" Height="52" Background="#0E1626" BorderBrush="#00F0FF" BorderThickness="1.5" CornerRadius="12" Margin="0,0,16,0">
                <TextBlock Text="⚡" FontSize="26" HorizontalAlignment="Center" VerticalAlignment="Center" Foreground="#00F0FF"/>
            </Border>
            <StackPanel Grid.Column="1" VerticalAlignment="Center">
                <TextBlock Text="Enlangg Setup Wizard" FontSize="20" FontWeight="Bold" Foreground="#FFFFFF"/>
                <TextBlock Text="Sovereign Natural English Language &amp; Compiler Toolchain" FontSize="12.5" Foreground="#94A3B8" Margin="0,2,0,0"/>
            </StackPanel>
        </Grid>

        <!-- Main Body -->
        <Border Grid.Row="1" Background="#0F172A" BorderBrush="#1E293B" BorderThickness="1" CornerRadius="10" Padding="20">
            <StackPanel x:Name="ConfigPanel">
                <TextBlock Text="Installation Directory" FontSize="13" FontWeight="SemiBold" Foreground="#E2E8F0" Margin="0,0,0,8"/>
                <Grid Margin="0,0,0,16">
                    <Grid.ColumnDefinitions>
                        <ColumnDefinition Width="*"/>
                        <ColumnDefinition Width="Auto"/>
                    </Grid.ColumnDefinitions>
                    <TextBox x:Name="TxtPath" Grid.Column="0" Height="36" Background="#090D16" Foreground="#00F0FF" BorderBrush="#334155" BorderThickness="1" 
                             VerticalContentAlignment="Center" Padding="10,0" FontSize="12.5" FontFamily="Consolas"/>
                    <Button x:Name="BtnBrowse" Grid.Column="1" Content="Browse..." Margin="8,0,0,0" Height="36" Width="85"/>
                </Grid>

                <TextBlock Text="Environment &amp; Integration" FontSize="13" FontWeight="SemiBold" Foreground="#E2E8F0" Margin="0,4,0,6"/>
                <CheckBox x:Name="ChkPath" Content="Automatically add Enlangg to System Environment Variables (PATH)" IsChecked="True"/>
                <TextBlock Text="Allows running 'enlangg' and 'enlng' commands directly from any terminal." FontSize="11.5" Foreground="#64748B" Margin="22,2,0,10"/>
                
                <CheckBox x:Name="ChkAssoc" Content="Register file associations for .enlng (Sovereign scripts)" IsChecked="True"/>
                <TextBlock Text="Enables direct execution and IDE file linking." FontSize="11.5" Foreground="#64748B" Margin="22,2,0,14"/>

                <!-- Progress Bar Section (Hidden initially) -->
                <ProgressBar x:Name="InstallProgress" Height="8" Background="#090D16" Foreground="#00F0FF" BorderThickness="0" Margin="0,10,0,8" Visibility="Collapsed"/>
                <TextBlock x:Name="TxtStatus" Text="Ready to install." FontSize="12" Foreground="#94A3B8" Visibility="Collapsed"/>
            </StackPanel>
        </Border>

        <!-- Footer Actions -->
        <Grid Grid.Row="2" Margin="0,20,0,0">
            <Grid.ColumnDefinitions>
                <ColumnDefinition Width="Auto"/>
                <ColumnDefinition Width="*"/>
                <ColumnDefinition Width="Auto"/>
            </Grid.ColumnDefinitions>
            <TextBlock Grid.Column="0" Text="v5.0.0 Sovereign Master" FontSize="11" Foreground="#475569" VerticalAlignment="Center"/>
            <StackPanel Grid.Column="2" Orientation="Horizontal">
                <Button x:Name="BtnCancel" Content="Cancel" Width="90" Height="36" Margin="0,0,10,0"/>
                <Button x:Name="BtnInstall" Content="Install Now" Width="120" Height="36" Background="#00F0FF" Foreground="#000000" BorderThickness="0"/>
                <Button x:Name="BtnFinish" Content="Finish" Width="100" Height="36" Background="#10B981" Foreground="#000000" BorderThickness="0" Visibility="Collapsed"/>
            </StackPanel>
        </Grid>
    </Grid>
</Window>
"@

$reader = [System.Xml.XmlNodeReader]::new($xaml)
$window = [System.Windows.Markup.XamlReader]::Load($reader)

$txtPath = $window.FindName("TxtPath")
$btnBrowse = $window.FindName("BtnBrowse")
$chkPath = $window.FindName("ChkPath")
$chkAssoc = $window.FindName("ChkAssoc")
$installProgress = $window.FindName("InstallProgress")
$txtStatus = $window.FindName("TxtStatus")
$btnInstall = $window.FindName("BtnInstall")
$btnCancel = $window.FindName("BtnCancel")
$btnFinish = $window.FindName("BtnFinish")

$txtPath.Text = $DefaultInstallBase

# Browse Button Action
$btnBrowse.Add_Click({
    $dialog = New-Object System.Windows.Forms.FolderBrowserDialog
    $dialog.Description = "Select Destination Folder for Enlangg"
    $dialog.SelectedPath = $txtPath.Text
    $dialog.ShowNewFolderButton = $true
    if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
        $txtPath.Text = $dialog.SelectedPath
    }
})

# Cancel Button
$btnCancel.Add_Click({
    $window.Close()
})

# Finish Button
$btnFinish.Add_Click({
    $window.Close()
})

# Install Button Action
$btnInstall.Add_Click({
    $target = $txtPath.Text.Trim()
    if ([string]::IsNullOrWhiteSpace($target)) {
        [System.Windows.MessageBox]::Show("Please select a valid installation directory.", "Enlangg Setup", "OK", "Warning")
        return
    }

    $btnInstall.IsEnabled = $false
    $btnBrowse.IsEnabled = $false
    $txtPath.IsEnabled = $false
    $chkPath.IsEnabled = $false
    $chkAssoc.IsEnabled = $false
    $btnCancel.IsEnabled = $false

    $installProgress.Visibility = [System.Windows.Visibility]::Visible
    $txtStatus.Visibility = [System.Windows.Visibility]::Visible
    $installProgress.Value = 10

    # Execute in background dispatcher to keep UI responsive
    [System.Windows.Threading.Dispatcher]::CurrentDispatcher.Invoke([Action]{
        try {
            Install-EnlanggCore $target $chkPath.IsChecked $chkAssoc.IsChecked `
                { param($msg) $txtStatus.Text = $msg; [System.Windows.Forms.Application]::DoEvents() } `
                { param($pct) $installProgress.Value = $pct; [System.Windows.Forms.Application]::DoEvents() }

            $txtStatus.Foreground = [System.Windows.Media.Brushes]::LightGreen
            $txtStatus.Text = "Installation Complete! Enlangg is now available in all terminals."

            $btnInstall.Visibility = [System.Windows.Visibility]::Collapsed
            $btnCancel.Visibility = [System.Windows.Visibility]::Collapsed
            $btnFinish.Visibility = [System.Windows.Visibility]::Visible
        } catch {
            $txtStatus.Foreground = [System.Windows.Media.Brushes]::Salmon
            $txtStatus.Text = "Installation Error: $($_.Exception.Message)"
            $btnCancel.IsEnabled = $true
        }
    })
})

# Show the GUI Window
$window.ShowDialog() | Out-Null
