$branch = (git branch --show-current) | Out-String
$commit = (git rev-parse --short $branch.Trim()) | Out-String
$date = (Get-Date) | Out-String

$branch = $branch.Trim()
$commit = $commit.Trim()
$date = $date.Trim()

$file = "Branch: $branch`nCommit: $commit`nBuilt: $date"
$code = "char Version[]=`"1.0.5`"; `r`nchar GitBranch[] = `"$Branch`"; `r`nchar GitCommit[] = `"$Commit`"; `r`nchar BuildDate[] = `"$date`"; `r`n"
Write-Host $file 
Out-File -FilePath version.h -InputObject $code
