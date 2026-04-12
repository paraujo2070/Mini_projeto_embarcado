================================================================================
GUIA DE ARQUITETURA REATIVA E INTERFACES AIDL - SENSOR DE CLIMA (froyo)
================================================================================

Este documento explica a implementação do daemon 'temp_froyo', que utiliza
programação reativa para fornecer dados de Temperatura, Umidade e Temperatura 
Ambiente de forma eficiente para o Android.

1. ARQUITETURA GERAL (Padrão Observer)
--------------------------------------
A arquitetura mudou de um modelo "Polling no App" para "Notificação no Daemon".
- ANTIGO: O App rodava um loop infinito e perguntava o valor ao Daemon (getTemperature).
- NOVO: O App se registra no Daemon, e o Daemon "empurra" (push) os dados para o 
  App apenas quando novos dados de hardware estão prontos.

2. INTERFACES AIDL (O Contrato)
-------------------------------

A) ITempListener.aidl (O Ouvinte)
   Define os callbacks que o Daemon chamará no App:
   - onClimateUpdate: Envia T, H e T_Ambient juntos (sincronizados).
   - onTemperatureUpdate / onHumidityUpdate / onAmbientTemperatureUpdate: 
     Envios individuais para casos de uso específicos.

B) ITemp.aidl (O Serviço)
   Gerencia a conexão com os Apps:
   - registerListener(listener, factor): Adiciona o App à lista de interessados.
     O 'factor' é usado para calcular a temperatura ambiente compensada.
   - unregisterListener(listener): Remove o App da lista.
   - (Mantém compatibilidade com os métodos 'get' síncronos).

3. IMPLEMENTAÇÃO C++ (O Motor)
------------------------------

A) TempHAL (Otimizações de Hardware):
   - Média Interna (AV_CONF): O sensor HTS221 foi configurado para realizar 32
     amostras internas antes de retornar o dado, eliminando ruídos de leitura.
   - Leitura em Bloco (Burst): Lemos 4 bytes de uma vez via I2C (T e H), 
     reduzindo o overhead no barramento.
   - Status Check: A leitura física só ocorre se o registrador de status (0x27)
     confirmar que o hardware terminou a conversão.

B) TempService (Gerenciamento de Threads):
   - Thread de Polling Dinâmica: A thread de leitura só existe enquanto houver
     pelo menos um App registrado. Se o último App sair, a thread é encerrada.
   - Thread-Safety: Um mutex protege a lista de clientes, permitindo que vários
     Apps se registrem/desregistrem simultaneamente sem causar crashes.

4. COMO USAR NO ANDROID (KOTLIN FLOW)
-------------------------------------

Para consumir este serviço de forma idiomática no Android, utilize o 'callbackFlow':

```kotlin
fun getClimateFlow(tempService: ITemp, factor: Float): Flow<ClimateData> = callbackFlow {
    // 1. Cria o listener (baseado no AIDL gerado)
    val listener = object : ITempListener.Stub() {
        override fun onClimateUpdate(t: Float, h: Float, tAmb: Float) {
            trySend(ClimateData(t, h, tAmb))
        }
        // Implementar outros métodos se necessário...
        override fun onTemperatureUpdate(t: Float) {}
        override fun onHumidityUpdate(h: Float) {}
        override fun onAmbientTemperatureUpdate(tAmb: Float) {}
    }

    // 2. Registra no serviço do sistema
    tempService.registerListener(listener, factor)

    // 3. Define a limpeza ao cancelar o Flow
    awaitClose {
        tempService.unregisterListener(listener)
    }
}.flowOn(Dispatchers.IO)
```

5. BENEFÍCIOS DESTA ABORDAGEM
-----------------------------
1. Economia de CPU: O App Android não gasta ciclos de CPU "vigiando" o sensor.
2. Precisão: O Daemon lê o hardware no momento exato em que o dado está pronto.
3. Menor Aquecimento: O barramento I2C é usado de forma otimizada.
4. Modularidade: Vários Apps podem ouvir o sensor simultaneamente com 
   fatores de compensação diferentes.

================================================================================
froyo - 2024
