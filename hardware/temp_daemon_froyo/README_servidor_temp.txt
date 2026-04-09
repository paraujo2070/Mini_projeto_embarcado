COMANDOS PARA O SERVIDOR REMOTO (TEMPERATURA COMPENSADA - froyo)

1. ACESSO E PREPARAÇÃO:
cd /data/aosp/android_course_environment/hardware/
mkdir -p temp_daemon_froyo

2. COMPILAÇÃO:
cd /data/aosp/android_course_environment
source build/new_envsetup.sh
lunch aosp_rpi5-bp4a-userdebug
croot
m temp_daemon_froyo

3. TESTE NA PLACA (service call):
adb shell
service list | grep temp

# Ler Temperatura Bruta (getTemperature)
service call temp_froyo 1

# Ler Umidade (getHumidity)
service call temp_froyo 2

# Ler Temperatura Ambiente com Fator 2.0 (getAmbientTemperature)
# O argumento é um float (fator), codificado em hex para o service call se necessário,
# mas via código AIDL basta passar o float normalmente.
service call temp_froyo 3 f 2.0
