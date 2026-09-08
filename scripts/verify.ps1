$ErrorActionPreference = 'Stop'
cmake -S . -B build-review -DBUILD_TESTING=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build build-review --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
ctest --test-dir build-review -C Release --output-on-failure
exit $LASTEXITCODE
