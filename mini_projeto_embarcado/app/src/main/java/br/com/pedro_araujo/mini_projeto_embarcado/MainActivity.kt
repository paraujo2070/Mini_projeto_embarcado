package br.com.pedro_araujo.mini_projeto_embarcado

import android.media.AudioManager
import android.media.ToneGenerator
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlin.random.Random

class MainActivity : AppCompatActivity() {

    companion object {
        private const val TAG = "GeniusGame"
    }

    private val senseHatController = SenseHatController()
    private val toneGenerator = ToneGenerator(AudioManager.STREAM_MUSIC, 100)

    private lateinit var tvStatus: TextView
    private lateinit var tvScore: TextView
    private lateinit var viewGreen: View
    private lateinit var viewRed: View
    private lateinit var viewBlue: View
    private lateinit var viewYellow: View
    private lateinit var btnStart: Button

    private val sequence = mutableListOf<JoystickDirection>()
    private var userIndex = 0
    private var isUserTurn = false
    private var score = 0
    private var currentDelay = 800L
    private var joystickPollingJob: Job? = null

    private val directions = listOf(
        JoystickDirection.UP,
        JoystickDirection.DOWN,
        JoystickDirection.LEFT,
        JoystickDirection.RIGHT
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        Log.d(TAG, "Atividade Criada")

        tvStatus = findViewById(R.id.tvStatus)
        tvScore = findViewById(R.id.tvScore)
        viewGreen = findViewById(R.id.viewGreen)
        viewRed = findViewById(R.id.viewRed)
        viewBlue = findViewById(R.id.viewBlue)
        viewYellow = findViewById(R.id.viewYellow)
        btnStart = findViewById(R.id.btnStart)

        btnStart.setOnClickListener {
            Log.i(TAG, "Botão Start clicado")
            startGame()
        }
    }

    private fun startGame() {
        Log.i(TAG, "Iniciando novo jogo")
        sequence.clear()
        score = 0
        currentDelay = 800L
        tvScore.text = "Pontuação: $score"
        addNewStep()
    }

    private fun addNewStep() {
        val nextDir = directions[Random.nextInt(directions.size)]
        sequence.add(nextDir)
        Log.d(TAG, "Novo passo adicionado à sequência: $nextDir. Tamanho total: ${sequence.size}")
        playSequence()
    }

    private fun playSequence() {
        lifecycleScope.launch {
            isUserTurn = false
            btnStart.isEnabled = false
            tvStatus.text = "OBSERVE..."
            
            Log.d(TAG, "Reproduzindo sequência. Aguardando 2000ms.")
            delay(2000)

            for ((index, dir) in sequence.withIndex()) {
                Log.v(TAG, "Piscando passo ${index + 1}: $dir")
                highlightDirection(dir)
                delay(currentDelay)
                resetHighlight()
                delay(currentDelay / 3)
            }

            Log.i(TAG, "Sequência terminada. Aguardando entrada do usuário.")
            tvStatus.text = "SUA VEZ!"
            userIndex = 0
            isUserTurn = true
            startJoystickPolling()
        }
    }

    private fun startJoystickPolling() {
        joystickPollingJob?.cancel()
        joystickPollingJob = lifecycleScope.launch {
            Log.d(TAG, "Monitoramento do joystick iniciado")
            var lastDir = JoystickDirection.NONE
            while (isUserTurn) {
                val currentDir = senseHatController.getJoystickState()
                
                if (currentDir != JoystickDirection.NONE && currentDir != lastDir) {
                    Log.d(TAG, "Entrada detectada: $currentDir")
                    handleUserInput(currentDir)
                } else if (currentDir == JoystickDirection.NONE && lastDir != JoystickDirection.NONE) {
                    resetHighlight()
                }
                
                lastDir = currentDir
                delay(50)
            }
        }
    }

    private fun handleUserInput(input: JoystickDirection) {
        highlightDirection(input)
        
        Log.d(TAG, "Comparando entrada: Recebido=$input | Esperado=${sequence[userIndex]} na posição $userIndex")
        
        if (input == sequence[userIndex]) {
            Log.i(TAG, "Entrada correta: $input no índice $userIndex")
            userIndex++
            if (userIndex == sequence.size) {
                score++
                Log.i(TAG, "Sequência completada! Pontuação atual: $score")
                tvScore.text = "Pontuação: $score"
                tvStatus.text = "MUITO BEM!"
                isUserTurn = false
                
                if (score % 3 == 0 && currentDelay > 300) {
                    currentDelay -= 100
                    Log.d(TAG, "Dificuldade aumentada! Novo delay: $currentDelay")
                }
                
                lifecycleScope.launch {
                    delay(1000)
                    resetHighlight()
                    addNewStep()
                }
            }
        } else {
            Log.e(TAG, "Entrada errada! Esperado ${sequence[userIndex]} mas recebeu $input")
            gameOver(sequence[userIndex])
        }
    }

    private fun gameOver(correctDir: JoystickDirection) {
        isUserTurn = false
        joystickPollingJob?.cancel()
        Log.w(TAG, "Fim de jogo. Pontuação final: $score")
        
        lifecycleScope.launch {
            toneGenerator.startTone(ToneGenerator.TONE_PROP_BEEP2, 1000)
            tvStatus.text = "GAME OVER!"
            
            repeat(3) {
                highlightDirection(correctDir)
                delay(300)
                resetHighlight()
                delay(300)
            }
            
            btnStart.isEnabled = true
            tvStatus.text = "PRESSIONE START"
        }
    }

    private fun highlightDirection(dir: JoystickDirection) {
        resetHighlight()
        when (dir) {
            JoystickDirection.UP -> {
                viewGreen.alpha = 1.0f
                toneGenerator.startTone(ToneGenerator.TONE_DTMF_1, 200)
            }
            JoystickDirection.DOWN -> {
                viewYellow.alpha = 1.0f
                toneGenerator.startTone(ToneGenerator.TONE_DTMF_4, 200)
            }
            JoystickDirection.LEFT -> {
                viewRed.alpha = 1.0f
                toneGenerator.startTone(ToneGenerator.TONE_DTMF_2, 200)
            }
            JoystickDirection.RIGHT -> {
                viewBlue.alpha = 1.0f
                toneGenerator.startTone(ToneGenerator.TONE_DTMF_3, 200)
            }
            else -> {}
        }
    }

    private fun resetHighlight() {
        viewGreen.alpha = 0.3f
        viewRed.alpha = 0.3f
        viewBlue.alpha = 0.3f
        viewYellow.alpha = 0.3f
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.d(TAG, "Atividade destruída")
        toneGenerator.release()
    }
}
