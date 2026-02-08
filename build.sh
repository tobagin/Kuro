#!/bin/bash
set -e

# Install required SDK
flatpak install --user --noninteractive org.gnome.Sdk//49 org.gnome.Platform//49

# Define the manifest to use
MANIFEST="packaging/io.github.tobagin.Kuro.yml"

if [[ "$1" == "--dev" ]]; then
    MANIFEST="packaging/io.github.tobagin.Kuro.Devel.yml"
    echo "Building Development version..."
else
    echo "Building Production version..."
fi

# Build the flatpak
flatpak-builder -v --user --install --force-clean build-dir "$MANIFEST"
