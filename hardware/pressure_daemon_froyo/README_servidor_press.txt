COMANDOS PARA O SERVIDOR REMOTO (PRESSÃO - froyo)

1. ACESSO E PREPARAÇÃO:
cd /data/aosp/android_course_environment/hardware/
mkdir -p pressure_daemon_froyo

2. COMPILAÇÃO:
cd /data/aosp/android_course_environment
source build/new_envsetup.sh
lunch aosp_rpi5-bp4a-userdebug
croot
m pressure_daemon_froyo

3. TESTE NA PLACA:
adb shell
service list | grep pressure
service call pressure_froyo 1
