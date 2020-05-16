adb root
adb remount
adb shell mount -o remount,rw /
adb push prebuilts/uinput-titan /system/bin/uinput-titan
adb push titan.rc /system/etc/init/
adb install -r prebuilts/KIkaInput.apk
adb shell setprop persist.sys.phh.mainkeys 1
adb shell pm enable com.iqqijni.bbkeyboard
adb shell ime enable com.iqqijni.bbkeyboard/.keyboard_service.view.HDKeyboardService
adb shell ime set com.iqqijni.bbkeyboard/.keyboard_service.view.HDKeyboardService
adb shell settings put secure show_ime_with_hard_keyboard 1
adb reboot
