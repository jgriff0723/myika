$ErrorActionPreference = "Stop"

$inputJson = [Console]::In.ReadToEnd()
if ([string]::IsNullOrWhiteSpace($inputJson)) {
    exit 0
}

try {
    $payload = $inputJson | ConvertFrom-Json
    $command = [string]$payload.tool_input.command
} catch {
    Write-Error "BLOCKED: could not parse Claude hook input JSON."
    exit 2
}

if ([string]::IsNullOrWhiteSpace($command)) {
    exit 0
}

$dangerousPatterns = @(
    "\bgit\s+push\b",
    "\bgit\s+reset\s+--hard\b",
    "\bgit\s+clean\s+-[A-Za-z]*f[A-Za-z]*\b",
    "\bgit\s+branch\s+-D\b",
    "\bgit\s+checkout\s+\.\b",
    "\bgit\s+restore\s+\.\b",
    "\bpush\s+--force\b",
    "\breset\s+--hard\b"
)

foreach ($pattern in $dangerousPatterns) {
    if ($command -match $pattern) {
        [Console]::Error.WriteLine("BLOCKED: '$command' matches dangerous pattern '$pattern'. The user has prevented you from doing this.")
        exit 2
    }
}

exit 0
