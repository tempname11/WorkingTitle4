$watcher = New-Object System.IO.FileSystemWatcher
$watcher.Path = "C:\Users\v7\Repositories\WorkingTitle4\out\build\x64-Debug\"
$watcher.Filter = "*.dll"
$watcher.NotifyFilter = [IO.NotifyFilters]::Attributes 

function OnChange {
  Write-Warning 'Change detected.'
  Start-Sleep 1.0
  $wshell = New-Object -ComObject wscript.shell;
  $ok = $wshell.AppActivate('WorkingTitleInstance')
  if ($ok) {
    $wshell.SendKeys('%r')
    Write-Warning 'Sent!'

    if ($false) {
      $ok = $wshell.AppActivate('Microsoft Visual Studio')
      if (!$ok) {
        Write-Warning '... could not change focus back ...'
      }
    }
  }
}

$changeTypes = [System.IO.WatcherChangeTypes]::Changed

try {
  while ($true) {
    $result = $watcher.WaitForChanged($changeTypes, 500)
    if ($result.TimedOut) { continue }
    OnChange
  }
}
finally {
  $watcher.Dispose()
}