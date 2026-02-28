package org.openemu.android

import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.lifecycleScope
import androidx.window.layout.FoldingFeature
import androidx.window.layout.WindowInfoTracker
import kotlinx.coroutines.launch

class MainActivity : ComponentActivity() {

    private var isFolded by mutableStateOf(false)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Monitor folding state
        lifecycleScope.launch {
            WindowInfoTracker.getOrCreate(this@MainActivity)
                .windowLayoutInfo(this@MainActivity)
                .collect { newLayoutInfo ->
                    val foldingFeature = newLayoutInfo.displayFeatures
                        .filterIsInstance<FoldingFeature>()
                        .firstOrNull()
                    
                    isFolded = foldingFeature?.state == FoldingFeature.State.HALF_OPENED
                }
        }

        setContent {
            OpenEmuTheme {
                Surface(modifier = Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.background) {
                    EmulatorLayout(isFolded)
                }
            }
        }
    }

    @Composable
    fun EmulatorLayout(isFolded: Boolean) {
        if (isFolded) {
            // Split screen for Foldable (Flex Mode)
            Column(modifier = Modifier.fillMaxSize()) {
                // Top half: Emulator Video
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxWidth()
                        .background(Color.Black),
                    contentAlignment = Alignment.Center
                ) {
                    EmulatorVideoSurface()
                }
                
                // Bottom half: Controls
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxWidth()
                        .background(Color.DarkGray),
                    contentAlignment = Alignment.Center
                ) {
                    OnScreenController()
                }
            }
        } else {
            // Full screen or standard layout
            Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                Text("Standard Layout (Unfolded)")
                // Standard UI would go here
            }
        }
    }

    @Composable
    fun EmulatorVideoSurface() {
        AndroidView(
            factory = { context ->
                SurfaceView(context).apply {
                    holder.addCallback(object : SurfaceHolder.Callback {
                        override fun surfaceCreated(holder: SurfaceHolder) {
                            nativeSetSurface(holder.surface)
                        }

                        override fun surfaceChanged(holder: SurfaceHolder, format: Int, w: Int, h: Int) {
                            nativeSetSize(w, h)
                        }

                        override fun surfaceDestroyed(holder: SurfaceHolder) {
                            nativeSetSurface(null)
                        }
                    })
                }
            },
            modifier = Modifier.fillMaxSize()
        )
    }

    @Composable
    fun OnScreenController() {
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Row {
                Button(onClick = { sendInput("UP") }) { Text("UP") }
            }
            Row {
                Button(onClick = { sendInput("LEFT") }) { Text("LEFT") }
                Spacer(modifier = Modifier.width(16.dp))
                Button(onClick = { sendInput("RIGHT") }) { Text("RIGHT") }
            }
            Row {
                Button(onClick = { sendInput("DOWN") }) { Text("DOWN") }
            }
            Spacer(modifier = Modifier.height(32.dp))
            Row {
                Button(onClick = { sendInput("B") }) { Text("B") }
                Spacer(modifier = Modifier.width(16.dp))
                Button(onClick = { sendInput("A") }) { Text("A") }
            }
        }
    }

    private fun sendInput(button: String) {
        nativeSendInput(button)
    }

    fun loadROM(path: String) {
        nativeLoadROM(path)
    }

    private external fun nativeSendInput(button: String)
    private external fun nativeLoadROM(path: String)
    private external fun nativeSetSurface(surface: Any?)
    private external fun nativeSetSize(width: Int, height: Int)
    private external fun stringFromJNI(): String

    companion object {
        init {
            System.loadLibrary("mgba")
        }
    }
}

@Composable
fun OpenEmuTheme(content: @Composable () -> Unit) {
    MaterialTheme(content = content)
}
