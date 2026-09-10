#!/system/bin/sh

# Simple diagnostics action for Magisk/KernelSU managers.
echo "=== Rucoy Tile Bot ==="
echo "Package: com.mmo.android"
echo "Target : world 187,445"
echo "Tap    : 886,361 display 0"
echo
echo "Recent logs:"
logcat -d -s RucoyTileBot:I '*:S' 2>/dev/null | tail -n 80
