adb root
adb remount
adb shell mount -o remount,rw /
adb install -r prebuilts/KIkaInput.apk
adb push mtk-pad.idc /system/usr/keylayout/mtk-pad.idc
adb shell rm -f /system/etc/init/titan.rc /system/bin/uinput-titan
adb shell setprop persist.sys.phh.mainkeys 1
adb shell pm enable com.iqqijni.bbkeyboard
adb shell ime enable com.iqqijni.bbkeyboard/.keyboard_service.view.HDKeyboardService
adb shell ime set com.iqqijni.bbkeyboard/.keyboard_service.view.HDKeyboardService
adb shell settings put secure show_ime_with_hard_keyboard 1
adb reboot
