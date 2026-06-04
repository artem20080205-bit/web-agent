#!/bin/bash
set -e
echo "=== Web-Agent Build Script ==="
OS="$(uname -s)"
echo "Platform: $OS"

if [ "$OS" = "Linux" ]; then
    if command -v apt-get &>/dev/null; then
        echo "Checking apt dependencies..."
        sudo apt-get install -y libcurl4-openssl-dev nlohmann-json3-dev cmake g++ 2>/dev/null || true
    fi
elif [ "$OS" = "Darwin" ]; then
    if command -v brew &>/dev/null; then
        brew install curl nlohmann-json cmake 2>/dev/null || true
    fi
fi

mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

echo
echo "=== Build OK ==="
echo "Binary: $(pwd)/webagent"
echo "Run:    ./webagent --config ../config/agent.json"
echo "Tests:  ctest --output-on-failure"
