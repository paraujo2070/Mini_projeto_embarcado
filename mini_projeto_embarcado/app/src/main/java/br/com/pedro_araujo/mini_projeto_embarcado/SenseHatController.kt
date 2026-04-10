package br.com.pedro_araujo.mini_projeto_embarcado

import android.os.IBinder
import android.util.Log
import com.android.joystick_froyo.IJoystick
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

enum class JoystickDirection {
    UP, DOWN, LEFT, RIGHT, PRESSED, NONE
}

class SenseHatController {

    companion object {
        private const val TAG = "SenseHatController"
    }

    private var mJoystickService: IJoystick? = null

    init {
        conectarAoServiceManager()
    }

    private fun conectarAoServiceManager() {
        try {
            Log.d(TAG, "Conectando ao joystick_froyo via ServiceManager...")
            val serviceManagerClass = Class.forName("android.os.ServiceManager")
            val getServiceMethod = serviceManagerClass.getMethod("getService", String::class.java)
            val binder = getServiceMethod.invoke(null, "joystick_froyo") as IBinder?

            if (binder != null) {
                mJoystickService = IJoystick.Stub.asInterface(binder)
                Log.d(TAG, "Conectado ao AIDL com sucesso!")
            } else {
                Log.e(TAG, "Falha: Binder 'joystick_froyo' não encontrado.")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Erro ao conectar AIDL: ${e.message}")
        }
    }

    suspend fun getJoystickState(): JoystickDirection = withContext(Dispatchers.IO) {
        try {
            if (mJoystickService == null) {
                conectarAoServiceManager()
            }

            val state = mJoystickService?.joystickState ?: -1
            
            if (state != 0 && state != -1) {
                Log.v(TAG, "Estado bruto AIDL: $state (Hex: 0x${Integer.toHexString(state)})")
            }

            when (state) {
                0 -> JoystickDirection.NONE
                1 -> JoystickDirection.UP      // 0x1
                4 -> JoystickDirection.DOWN    // 0x4
                16 -> JoystickDirection.LEFT   // 0x10
                2 -> JoystickDirection.RIGHT   // 0x2
                8 -> JoystickDirection.PRESSED // 0x8
                else -> JoystickDirection.NONE
            }
        } catch (e: Exception) {
            Log.e(TAG, "Erro ao chamar getJoystickState via AIDL: ${e.message}")
            JoystickDirection.NONE
        }
    }
}
