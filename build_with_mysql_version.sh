#!/bin/bash

# Build and test the extension with different MySQL versions
# Usage: ./build_with_mysql_version.sh [8.0|8.4|9.4]

set -e

MYSQL_VERSION=${1:-"9.4"}
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "🚀 Building PHP MySQL Query Parser Extension"
echo "============================================="
echo "Target MySQL Version: $MYSQL_VERSION"
echo "Script Directory: $SCRIPT_DIR"
echo ""

# Function to build with specific MySQL version
build_with_mysql() {
    local version=$1
    local mysql_path=""
    local mysql_config=""
    
    case $version in
        "8.0")
            mysql_path="/opt/homebrew/opt/mysql@8.0"
            mysql_config="/opt/homebrew/opt/mysql@8.0/bin/mysql_config"
            ;;
        "8.4")
            mysql_path="/opt/homebrew/opt/mysql@8.4"  
            mysql_config="/opt/homebrew/opt/mysql@8.4/bin/mysql_config"
            ;;
        "9.4")
            mysql_path="/opt/homebrew/opt/mysql"
            mysql_config="/opt/homebrew/bin/mysql_config"
            ;;
        *)
            echo "❌ Unsupported MySQL version: $version"
            echo "Supported versions: 8.0, 8.4, 9.4"
            exit 1
            ;;
    esac
    
    # Check if MySQL version is installed
    if [[ ! -f "$mysql_config" ]]; then
        echo "❌ MySQL $version not found at $mysql_config"
        echo "Install it with: brew install mysql@$version"
        exit 1
    fi
    
    echo "📋 MySQL $version Information:"
    echo "   Path: $mysql_path"
    echo "   Config: $mysql_config"
    echo "   Version: $($mysql_config --version)"
    echo "   Include: $($mysql_config --include)"
    echo "   Libs: $($mysql_config --libs)"
    echo ""
    
    # Clean previous build
    echo "🧹 Cleaning previous build..."
    make clean 2>/dev/null || true
    phpize --clean 2>/dev/null || true
    
    # Set environment for this MySQL version
    export MYSQL_CONFIG="$mysql_config"
    export CPPFLAGS="-I$mysql_path/include"
    export LDFLAGS="-L$mysql_path/lib"
    export LIBRARY_PATH="$mysql_path/lib:/opt/homebrew/lib:$LIBRARY_PATH"
    
    echo "🔧 Building extension with MySQL $version..."
    
    # Generate configure script
    phpize
    
    # Configure with specific MySQL version
    ./configure --enable-mysql-qp \
                --with-mysql-config="$mysql_config"
    
    # Build
    make
    
    echo "✅ Build completed successfully with MySQL $version"
    echo ""
}

# Function to run tests
run_tests() {
    local version=$1
    
    echo "🧪 Running tests with MySQL $version..."
    
    # Run official test suite
    echo "   Running official test suite..."
    if make test; then
        echo "   ✅ Official tests passed"
    else
        echo "   ❌ Official tests failed"
        return 1
    fi
    
    # Run version compatibility test
    echo "   Running version compatibility test..."
    if php -dextension=./modules/mysql_qp.so test_mysql_versions.php; then
        echo "   ✅ Version compatibility test completed"
    else
        echo "   ❌ Version compatibility test failed"
        return 1
    fi
    
    echo ""
}

# Function to create test report
create_report() {
    local version=$1
    local report_file="compatibility_report_mysql_${version}.txt"
    
    echo "📊 Creating compatibility report..."
    
    {
        echo "MySQL $version Compatibility Report"
        echo "=================================="
        echo "Generated: $(date)"
        echo "Extension: PHP MySQL Query Parser"
        echo "MySQL Client Version: $($MYSQL_CONFIG --version)"
        echo ""
        
        echo "Build Information:"
        echo "-----------------"
        echo "MySQL Config: $MYSQL_CONFIG"
        echo "Include Path: $($MYSQL_CONFIG --include)"
        echo "Library Path: $($MYSQL_CONFIG --libs)"
        echo ""
        
        echo "Test Results:"
        echo "------------"
        php -dextension=./modules/mysql_qp.so test_mysql_versions.php
        
    } > "$report_file"
    
    echo "📋 Report saved to: $report_file"
}

# Main execution
main() {
    cd "$SCRIPT_DIR"
    
    # Build with specified MySQL version
    build_with_mysql "$MYSQL_VERSION"
    
    # Run tests
    if run_tests "$MYSQL_VERSION"; then
        echo "🎉 All tests passed with MySQL $MYSQL_VERSION!"
    else
        echo "❌ Some tests failed with MySQL $MYSQL_VERSION"
        exit 1
    fi
    
    # Create compatibility report
    create_report "$MYSQL_VERSION"
    
    echo ""
    echo "🏁 Build and test completed successfully!"
    echo "   Extension built with: MySQL $MYSQL_VERSION"
    echo "   Extension file: ./modules/mysql_qp.so"
    echo "   Test all functions: php -dextension=./modules/mysql_qp.so test_mysql_versions.php"
    echo ""
}

# Help message
if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    echo "Usage: $0 [mysql_version]"
    echo ""
    echo "Build and test PHP MySQL Query Parser Extension with specific MySQL version"
    echo ""
    echo "Arguments:"
    echo "  mysql_version    MySQL version to build against (8.0, 8.4, 9.4)"
    echo "                   Default: 9.4"
    echo ""
    echo "Examples:"
    echo "  $0 8.0           Build with MySQL 8.0"
    echo "  $0 8.4           Build with MySQL 8.4" 
    echo "  $0 9.4           Build with MySQL 9.4 (default)"
    echo ""
    echo "Prerequisites:"
    echo "  brew install mysql@8.0    # For MySQL 8.0 testing"
    echo "  brew install mysql@8.4    # For MySQL 8.4 testing"
    echo "  brew install mysql        # For MySQL 9.4 testing"
    exit 0
fi

# Run main function
main