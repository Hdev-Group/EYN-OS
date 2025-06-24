#!/bin/bash

# Update version script for EYN-OS
# Usage: ./update_version.sh <major>.<minor>
# Example: ./update_version.sh 0.09

if [ $# -ne 1 ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 0.09"
    exit 1
fi

VERSION=$1

# Validate version format (simple check for x.y format)
if ! [[ $VERSION =~ ^[0-9]+\.[0-9]+$ ]]; then
    echo "Error: Version must be in format x.y (e.g., 0.09)"
    exit 1
fi

echo "Updating version to $VERSION..."

# Update shell.c
sed -i "s/ver\. 0\.[0-9]*/ver. $VERSION/g" src/utilities/shell/shell.c

# Update kernel.c
sed -i "s/EYN-OS v0\.[0-9]*/EYN-OS v$VERSION/g" src/entry/kernel.c

echo "Version updated successfully!"
echo "Files modified:"
echo "  - src/utilities/shell/shell.c"
echo "  - src/entry/kernel.c" 