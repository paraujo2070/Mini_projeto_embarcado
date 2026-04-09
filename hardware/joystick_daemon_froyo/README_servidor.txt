COMANDOS PARA O SERVIDOR REMOTO (androidbuilder.ic.unicamp.br)

1. ACESSO E PREPARAÇÃO:
cd /data/aosp/android_course_environment/hardware/
mkdir -p joystick_daemon_froyo

2. TRANSFERÊNCIA:
Copie os arquivos da pasta 'hardware/joystick_daemon_froyo/' local para a pasta criada no servidor.

3. COMPILAÇÃO:
cd /data/aosp/android_course_environment
source build/new_envsetup.sh
lunch aosp_rpi5-bp4a-userdebug
croot
m joystick_daemon_froyo

4. ARQUIVOS GERADOS (PARA ADB PUSH):
- HAL: out/target/product/rpi5/vendor/lib64/libjoystick_hal_froyo.so  ->  /vendor/lib64/
- NDK AIDL: out/target/product/rpi5/system/lib64/joystick_aidl_froyo-ndk.so  ->  /system/lib64/
- Daemon: out/target/product/rpi5/system/bin/joystick_daemon_froyo  ->  /system/bin/
- Init RC: out/target/product/rpi5/system/etc/init/joystick_froyo.rc  ->  /system/etc/init/

5. TESTE NO TERMINAL DA PLACA:
adb shell
service list | grep joystick
service call joystick_froyo 1
