package br.com.pedro_araujo.mini_projeto_embarcado

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.os.Build
import android.os.IBinder
import android.os.RemoteException
import android.util.Log
import androidx.core.app.NotificationCompat
import com.android.joystick_froyo.IJoystick

class JoystickService : Service() {

    companion object {
        const val ACTION_JOYSTICK_UPDATE = "br.com.pedro_araujo.mini_projeto_embarcado.JOYSTICK_UPDATE"
        private const val CHANNEL_ID = "JoystickServiceChannel"
        private const val NOTIFICATION_ID = 2
        private const val TAG = "JoystickService"
    }

    private var mJoystickService: IJoystick? = null
    private var isRunning = false

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val notification = createNotification("Monitorando Joystick via AIDL...")
        startForeground(NOTIFICATION_ID, notification)

        if (!isRunning) {
            isRunning = true
            conectarAoDaemon()
        }
        return START_STICKY
    }

    private fun conectarAoDaemon() {
        Thread {
            try {
                Log.d(TAG, "Conectando ao serviço joystick_froyo...")
                val serviceManagerClass = Class.forName("android.os.ServiceManager")
                val getServiceMethod = serviceManagerClass.getMethod("getService", String::class.java)
                val binder = getServiceMethod.invoke(null, "joystick_froyo") as IBinder?

                if (binder != null) {
                    mJoystickService = IJoystick.Stub.asInterface(binder)
                    Log.d(TAG, "Serviço AIDL conectado com sucesso!")
                    loopDeLeitura()
                } else {
                    Log.e(TAG, "Serviço 'joystick_froyo' não encontrado!")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Erro na conexão AIDL: ${e.message}")
            }
        }.start()
    }

    private fun loopDeLeitura() {
        while (isRunning && mJoystickService != null) {
            try {
                val state = mJoystickService?.joystickState ?: 0
                
                if (state != 0 && state != -1) {
                    val intent = Intent(ACTION_JOYSTICK_UPDATE).apply {
                        setPackage(packageName)
                        putExtra("state", state)
                    }
                    sendBroadcast(intent)
                    Log.v(TAG, "Joystick via AIDL: $state")
                }
                
                Thread.sleep(50) // Polling de 50ms
            } catch (e: RemoteException) {
                Log.e(TAG, "Erro de comunicação AIDL: ${e.message}")
                break
            } catch (e: InterruptedException) {
                break
            }
        }
    }

    private fun createNotification(content: String): Notification {
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("Joystick Genius Service")
            .setContentText(content)
            .setSmallIcon(android.R.drawable.ic_dialog_info)
            .setOngoing(true)
            .build()
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(CHANNEL_ID, "Joystick Monitor", NotificationManager.IMPORTANCE_LOW)
            getSystemService(NotificationManager::class.java)?.createNotificationChannel(channel)
        }
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onDestroy() {
        isRunning = false
        super.onDestroy()
    }
}
