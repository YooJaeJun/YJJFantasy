# 게임을 창 모드로 띄워 자동 테스트를 돌리고, 로그 요약과 스크린샷 경로를 보여준다.
#   powershell -File Tools/RunGame.ps1 -Mode tour    # 명소 스크린샷 (Saved/Screenshots/Tour_*.png)
#   powershell -File Tools/RunGame.ps1 -Mode play    # 자동 플레이 검증 ([AutoPilot] PASS/FAIL)
#   powershell -File Tools/RunGame.ps1 -Mode none    # 그냥 플레이
param(
    [ValidateSet("tour", "play", "none")]
    [string]$Mode = "tour",
    [string]$EngineDir = "D:\source\repos\UnrealEngine\Engine",
    [int]$ResX = 1600,
    [int]$ResY = 900,
    [int]$TimeoutSec = 1500
)

$ProjectDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$Project = Join-Path $ProjectDir "YJJFantasy.uproject"
$Editor = Join-Path $EngineDir "Binaries\Win64\UnrealEditor.exe"
$Log = Join-Path $ProjectDir "Saved\Logs\RunGame.log"
$Shots = Join-Path $ProjectDir "Saved\Screenshots"

$Flags = @()
if ($Mode -eq "tour") { $Flags += "-yjjtour", "-yjjquit"; Remove-Item (Join-Path $Shots "Tour_*.png") -ErrorAction SilentlyContinue }
if ($Mode -eq "play") { $Flags += "-yjjplay", "-yjjquit"; Remove-Item (Join-Path $Shots "Play_*.png") -ErrorAction SilentlyContinue }

$Args = @("`"$Project`"", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-WinX=40", "-WinY=40", "-nosplash", "-unattended", "-log", "-abslog=`"$Log`"") + $Flags
$Process = Start-Process -FilePath $Editor -ArgumentList $Args -PassThru
if ($Mode -ne "none") {
    if (-not $Process.WaitForExit($TimeoutSec * 1000)) {
        $Process.Kill()
        Write-Host "Timed out"
    }
    Select-String -Path $Log -Pattern "\[AutoPilot\]|\[World\]|\[Adventure\]|\[Hero\]|Error:|Ensure condition failed|Fatal|Failed to compile" | ForEach-Object { $_.Line }
}
