package br.com.pedro_araujo.mini_projeto_embarcado

import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

enum class JoystickDirection {
    UP, DOWN, LEFT, RIGHT, PRESSED, NONE
}

class SenseHatController {

    companion object {
        private const val TAG = "SenseHatController"
    }

    suspend fun getJoystickState(): JoystickDirection = withContext(Dispatchers.IO) {
        var process: Process? = null
        try {
            process = ProcessBuilder("sensehat_cli", "joystick")
                .redirectErrorStream(true)
                .start()
            val output = process.inputStream.bufferedReader().use { reader ->
                reader.readText()
            }
            process.waitFor()
            val result = parseJoystickOutput(output.trim())
            if (result != JoystickDirection.NONE) {
                Log.d(TAG, "Estado do Joystick detectado (corrigido): $result | Saída bruta: ${output.trim()}")
            }
            result
        } catch (e: Exception) {
            Log.e(TAG, "Erro ao ler sensor do joystick: ${e.message}", e)
            JoystickDirection.NONE
        } finally {
            process?.destroy()
        }
    }

    private fun parseJoystickOutput(output: String): JoystickDirection {
        // Formato da saída: "Joystick: Up=0 Down=0 Left=0 Right=0 Pressed=0 (Raw=0x00)"
        // Invertendo UP e DOWN conforme solicitado pelo usuário
        return when {
            output.contains("Up=1") -> JoystickDirection.DOWN // Invertido: Físico Up -> Lógica Down
            output.contains("Down=1") -> JoystickDirection.UP // Invertido: Físico Down -> Lógica Up
            output.contains("Left=1") -> JoystickDirection.LEFT
            output.contains("Right=1") -> JoystickDirection.RIGHT
            output.contains("Pressed=1") -> JoystickDirection.PRESSED
            else -> JoystickDirection.NONE
        }
    }
}
