#!/system/bin/sh

SKIPUNZIP=0

ui_print "*******************************"
ui_print "  Rucoy Tile Bot - Zygisk"
ui_print "*******************************"
ui_print "Target package : com.mmo.android"
ui_print "World target   : 187,445"
ui_print "Tap coordinate : 886,361"
ui_print "Architecture   : arm64-v8a"

ABI="$(getprop ro.product.cpu.abi)"
case "$ABI" in
  arm64-v8a|arm64*) ;;
  *)
    ui_print "! Unsupported ABI: $ABI"
    abort "This build only contains arm64-v8a.so"
    ;;
esac

ui_print "- Reboot after installation"
ui_print "- Check logs with: logcat -s RucoyTileBot"
