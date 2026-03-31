#!/bin/bash

# Build script for Java Banking Client
# Compiles all Java source files and creates executable JAR

echo "╔═══════════════════════════════════════════════════════════╗"
echo "║       Building Distributed Banking System Client          ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

# Configuration
SRC_DIR="src"
BUILD_DIR="build"
DIST_DIR="dist"
JAR_NAME="BankingClient.jar"
MAIN_CLASS="client.Main"

# Clean previous build
echo "🧹 Cleaning previous build..."
rm -rf "$BUILD_DIR" "$DIST_DIR"
mkdir -p "$BUILD_DIR" "$DIST_DIR"

# Compile Java files
echo "🔨 Compiling Java sources..."
javac -d "$BUILD_DIR" -sourcepath "$SRC_DIR" "$SRC_DIR/client/Main.java"

if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi

echo "✓ Compilation successful"

# Create manifest
echo "📝 Creating manifest..."
cat > "$BUILD_DIR/MANIFEST.MF" << EOF
Manifest-Version: 1.0
Main-Class: $MAIN_CLASS
EOF

# Create JAR
echo "📦 Creating JAR file..."
cd "$BUILD_DIR"
jar cfm "../$DIST_DIR/$JAR_NAME" MANIFEST.MF $(find . -name "*.class")

if [ $? -ne 0 ]; then
    echo "❌ JAR creation failed!"
    exit 1
fi

cd ..
echo "✓ JAR created: $DIST_DIR/$JAR_NAME"

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║                  BUILD SUCCESSFUL!                        ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""
echo "To run the client:"
echo "  java -jar $DIST_DIR/$JAR_NAME <server_host> <server_port> <semantics>"
echo ""
echo "Example:"
echo "  java -jar $DIST_DIR/$JAR_NAME localhost 2222 at-least-once"
echo ""
echo "Or run directly from build directory:"
echo "  java -cp $BUILD_DIR $MAIN_CLASS localhost 2222 at-most-once"
echo ""
